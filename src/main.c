#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <pico/stdlib.h>
#include <pico/bootrom.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <timers.h>

#include "tkjhat/sdk.h"
#include "tkjhat/buzzer.h"

#include <tusb.h>
#include "usbSerialDebug/helper.h"

#include "melodies.h"
#include "imu.h"
//#include "sound.h"
#include "gesture.h"
#include "display.h"

#define DEFAULT_STACK_SIZE 1024
#define CDC_ITF_TX      1
#define MORSE_BUFFER_SIZE 128
#define DEBOUNCE_MS 30

typedef enum {
    SOUND_DOT = 0,
    SOUND_DASH,
    SOUND_SONG_SIK_HYMN
} sound_event_t;

typedef enum { 
    STATE_IDLE = 0,
    STATE_RECORDING,
    STATE_SENDING,
} system_state_t;

// volatile because modified inside an interrupt
volatile system_state_t program_state = STATE_IDLE;

static volatile bool btn_pressed = false;

SemaphoreHandle_t i2c_mutex;

QueueHandle_t imu_queue;
QueueHandle_t gesture_queue;
QueueHandle_t sound_queue;
QueueHandle_t display_queue;

static TimerHandle_t btn_debounce_timer;


void tud_cdc_rx_cb(uint8_t itf) {
    char rx_buf[CFG_TUD_CDC_RX_BUFSIZE];
    uint32_t rx_len = 0;
    rx_len = tud_cdc_n_read(itf, rx_buf, sizeof(rx_buf));

    // Only process Morse on CDC1 (still read CDC0 above so its buffer doesn't stall)
    if (itf == CDC_ITF_TX && rx_len > 0) {
        for (uint32_t i = 0; i < rx_len; i++) {
            char c = rx_buf[i];

            // send our received char to display
            display_data_t dp = { DISPLAY_CMD_SYMBOL_RX, c };
            xQueueSend(display_queue, &dp, 0);

            // if we are recording, do not play any sounds.
            if (program_state == STATE_RECORDING) continue;

            if (c == '.') {
                sound_event_t snd = SOUND_DOT;
                xQueueSend(sound_queue, &snd, 0);
            } else if (c == '-') {
                sound_event_t snd = SOUND_DASH;
                xQueueSend(sound_queue, &snd, 0);
            }
        }
    }
}

// Runs 30ms after LAST bounce
static void btn_timer_cb(TimerHandle_t xTimer) {
    (void)xTimer;
    btn_pressed = gpio_get(SW1_PIN);
}

// just restart the debounce timer on any edge
static void btn_fxn(uint gpio, uint32_t eventMask) {
    xTimerResetFromISR(btn_debounce_timer, NULL);
}

// General state machine / processing task
static void morse_task(void *arg) {
    (void)arg;

    gest_t gesture;
    sound_event_t snd;
    const char gest_char_arr[] = {'.', '-', ' '};
    char buf[MORSE_BUFFER_SIZE + 1] = ""; //to include null termination
    int index = 0;
    char sik_morse[] = "... .. -.- ";

    for(;;) {
        
        switch (program_state) {
            case STATE_RECORDING:
                
                // 20 ms xTicksToWait here so we are not stuck sleeping !!! very important
                if (xQueueReceive(gesture_queue, &gesture, btn_pressed ? pdMS_TO_TICKS(20) : 0) == pdTRUE) {
                    
                    // quick fix for leftover pause from last session
                    if (gesture == GEST_PAUSE && index == 0) break;

                    // send corresponding sound to buzzer
                    if (gesture == GEST_RIGHT) {
                        snd = SOUND_DOT;
                        xQueueSend(sound_queue, &snd, 0);
                    }

                    else if (gesture == GEST_LEFT) {
                        snd = SOUND_DASH;
                        xQueueSend(sound_queue, &snd, 0);
                    }
                    
                    // send corresponding symbol to display
                    char symbol = gest_char_arr[gesture];
                    display_data_t dp = { DISPLAY_CMD_SYMBOL_TX, symbol };
                    xQueueSend(display_queue, &dp, 0);

                    if(index < MORSE_BUFFER_SIZE){
                        buf[index] = symbol;
                        buf[index + 1] = '\0';
                        index++;
                    }

                    //correct size first to avoid running strcmp
                    if(index == 11 && strcmp(buf, sik_morse) == 0){
                        snd = SOUND_SONG_SIK_HYMN;
                        xQueueSend(sound_queue, &snd, 0);
                    }
                }
                else if (!btn_pressed) {
                    program_state = STATE_SENDING;
                }

                break;

            case STATE_SENDING:

                // if something to send, send it here
                if (index > 0 && tud_cdc_n_connected(CDC_ITF_TX)) {
                    tud_cdc_n_write(CDC_ITF_TX, buf, index);
                    tud_cdc_n_write(CDC_ITF_TX, "\n", 1);
                    tud_cdc_n_write_flush(CDC_ITF_TX);

                    // blink red led once to indicate sending (should bring +0.5pts)
                    set_red_led_status(true);
                    vTaskDelay(pdMS_TO_TICKS(150));
                    set_red_led_status(false);
                }
                
                // state_sending is only entered from previous state_recording,
                // so we reset everything here, to be ready for next recording
                
                // reset gestures so that leftovers don't appear next time we start recording
                // (which might be the case if state changed while a gesture was being processed)
                xQueueReset(gesture_queue);

                // reset display-related stuff
                index = 0;
                buf[0] = '\0';
                display_data_t dp = { DISPLAY_CMD_RESET };
                xQueueSend(display_queue, &dp, 0);

                program_state = STATE_IDLE;
                break;

            default:
                if (btn_pressed) {
                    xQueueReset(imu_queue);
                    xQueueReset(gesture_queue);
                    program_state = STATE_RECORDING;
                }
                else {
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
                break;
        }
    }

}

static void imu_task(void *arg){
    (void)arg;

    imu_data_t raw_reading = {0};

    for(;;) {

        if (program_state != STATE_RECORDING) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if(xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE){
            // OLED might hold for long time
            // So we need to check state again before using
            if (program_state != STATE_RECORDING) {
                xSemaphoreGive(i2c_mutex);
                continue;
            }
            imu_read_raw(&raw_reading);
            xSemaphoreGive(i2c_mutex);
            xQueueSend(imu_queue, &raw_reading, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void buzzer_task(void *arg) {

    sound_event_t event;
    buzzer_init(BUZZER_PIN);
    buzzer_set_volume(1);

    while (1) {
        if (xQueueReceive(sound_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event) {
                case SOUND_DOT:
                    //play_note(&dot);
                    buzzer_tone_async(dot.freq_hz, dot.duration_ms);
                    break;
                case SOUND_DASH:
                    //play_note(&line);
                    buzzer_tone_async(line.freq_hz, line.duration_ms);
                    break;
                case SOUND_SONG_SIK_HYMN:
                    //play_song(sik_hymn);
                    buzzer_play_melody_async(sik_hymn, sik_hymn_length);
                    break;
            }
            //because using non-blocking implementation, wait until done with last item to consume next
            buzzer_wait_done();
        }
    }
}


static void usbTask(void *arg) {
    (void)arg;
    while (1) {
        tud_task(); // With FreeRTOS wait for events
    }
}

int main() {
    
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    //init_buzzer();

    init_sw1();
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, btn_fxn);

    init_red_led();

    init_imu();
    start_imu();

    init_display();
    clear_display();

    i2c_mutex = xSemaphoreCreateMutex();
    configASSERT(i2c_mutex != NULL);

    imu_queue     = xQueueCreate(20, sizeof(imu_data_t));
    gesture_queue = xQueueCreate(10, sizeof(gest_t));
    sound_queue   = xQueueCreate(512, sizeof(sound_event_t));
    display_queue = xQueueCreate(512, sizeof(display_data_t)); //has to be as large as the recieved message buffer if we want to display all recieved chars

    TaskHandle_t hImuTask, hUSB, hBuzzerTask, hGestureProcessTask, hMorseTask, hDisplayTask;

    xTaskCreate(usbTask, "usb", 2048, NULL, 3, &hUSB);
    #if (configNUMBER_OF_CORES > 1)
        vTaskCoreAffinitySet(hUSB, 1u << 0);
    #endif
    
    BaseType_t result = xTaskCreate(imu_task, // (en) Task function
                "imu",                        // (en) Name of the task 
                DEFAULT_STACK_SIZE,           // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                         // (en) Arguments of the task 
                2,                            // (en) Priority of this task
                &hImuTask);                   // (en) A handle to control the execution of this task

    if(result != pdPASS) return 0;
    
    static gesture_task_config_t gesture_cfg;
    gesture_cfg.imu_queue     = imu_queue;
    gesture_cfg.gesture_queue = gesture_queue;

    result = xTaskCreate(gesture_process_task, "gesture_processing", DEFAULT_STACK_SIZE, &gesture_cfg, 2, &hGestureProcessTask);
    if (result != pdPASS) return 0;

    result = xTaskCreate(display_task, "display", DEFAULT_STACK_SIZE, &display_queue, 2, &hDisplayTask);
    if (result != pdPASS) return 0;

    result = xTaskCreate(buzzer_task, "buzzer", 1024, NULL, 2, &hBuzzerTask);
    if (result != pdPASS) return 0;

    result = xTaskCreate(morse_task, "morse_task", DEFAULT_STACK_SIZE, NULL, 2, &hMorseTask);
    if(result != pdPASS) return 0;

    btn_debounce_timer = xTimerCreate(
        "BtnDebounce",
        pdMS_TO_TICKS(DEBOUNCE_MS),
        pdFALSE, // no auto reload
        NULL,
        btn_timer_cb
    );

    tusb_init();
    usb_serial_init();

    vTaskStartScheduler();
    
    return 0;
}

