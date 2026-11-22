#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <pico/stdlib.h>
#include <pico/bootrom.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

#include <tusb.h>
#include "usbSerialDebug/helper.h"

#include "imu.h"
#include "sound.h"
#include "gesture.h"
#include "display.h"

#define DEFAULT_STACK_SIZE 1024
#define CDC_ITF_TX      1

typedef enum { 
    STATE_IDLE = 0,
    STATE_RECORDING,
    STATE_SENDING,
} system_state_t;

// volatile because modified inside an interrupt
volatile system_state_t program_state = STATE_RECORDING;

SemaphoreHandle_t i2c_mutex;

QueueHandle_t imu_queue;
QueueHandle_t gesture_queue;
QueueHandle_t sound_queue;
QueueHandle_t display_queue;

static void btn_fxn(uint gpio, uint32_t eventMask) {
    // TODO: 
    // catch GPIO_IRQ_EDGE_RISE and set state to recording
    // catch GPIO_IRQ_EDGE_FALL and set state to sending (only if the current state is recording, else do nothing)
    // i assume we need to software debounce
    // Note, we must use ISR special functions like xTaskGetTickCountFromISR()
}

// General state machine / processing task
static void morse_task(void *arg) {
    (void)arg;

    gest_t gesture;
    sound_event_t snd;
    const char gest_char_arr[] = {'.', '-', ' '};
    for(;;) {
        
        switch (program_state) {
            case STATE_RECORDING:
                // 20 ms xTicksToWait here so we are not stuck sleeping !!! very important
                if (xQueueReceive(gesture_queue, &gesture, pdMS_TO_TICKS(20)) == pdTRUE) {
                    
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
                    display_data_t dp = { symbol, false };

                    xQueueSend(display_queue, &dp, 0);
                    char buf[128];
                    //int len = snprintf(buf, sizeof(buf), "Gyro: (%f, %f, %f)\nAccl: (%f, %f, %f)\nTime:%ld\n", gd.x, gd.y, gd.z, ad.x, ad.y, ad.z, t);

                    int len = snprintf(buf, sizeof(buf), "%d", gesture);
                    
                    if (len > 0) {
                        // Ensure we don't try to write more than the buffer size if truncated
                        uint32_t write_len = (len < sizeof(buf)) ? len : (sizeof(buf) - 1);
                        
                        tud_cdc_write(buf, write_len);
                        tud_cdc_write_flush(); // Forces the USB packet to be sent immediately
                    }
                }
                break;

            case STATE_SENDING:

                // if something to send, send it
                // then go state_idle

                break;
            
            default:
                vTaskDelay(pdMS_TO_TICKS(20));
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
            imu_read_raw(&raw_reading);
            xSemaphoreGive(i2c_mutex);
            xQueueSend(imu_queue, &raw_reading, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // TODO: try different values here
    }
}

void buzzer_task(void *arg) {

    sound_event_t event;

    while (1) {
        if (xQueueReceive(sound_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event) {
                case SOUND_DOT:
                    play_note(&dot);
                    break;
                case SOUND_DASH:
                    play_note(&line);
                    break;
                case SOUND_SONG_SIK_HYMN:
                    play_song(sik_hymn);
                    break;
            }
        }
    }
}

static void print_task(void *arg){
    (void)arg;
    
    while(1){
        tight_loop_contents();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void usbTask(void *arg) {
    (void)arg;
    while (1) {
        tud_task(); // With FreeRTOS wait for events
    }
}

int main() {
    
    //stdio_init_all();

    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    init_buzzer();

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
    sound_queue   = xQueueCreate(5,  sizeof(sound_event_t));
    display_queue = xQueueCreate(16, sizeof(display_data_t));

    TaskHandle_t hImuTask, hPrintTask, hUSB, hBuzzerTask, hGestureProcessTask, hMorseTask, hDisplayTask;

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

    if(result != pdPASS) {
        printf("IMU task creation failed\n");
        return 0;
    }
    
    static gesture_task_config_t gesture_cfg;
    gesture_cfg.imu_queue     = imu_queue;
    gesture_cfg.gesture_queue = gesture_queue;

    result = xTaskCreate(gesture_process_task, "gesture_processing", DEFAULT_STACK_SIZE, &gesture_cfg, 2, &hGestureProcessTask);

    if(result != pdPASS) {
        printf("Gesture processing Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(display_task, "display", DEFAULT_STACK_SIZE, &display_queue, 2, &hDisplayTask);

    if(result != pdPASS) {
        printf("Display Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(buzzer_task, "buzzer", 1024, NULL, 2, &hBuzzerTask);

    if(result != pdPASS) {
        printf("Buzzer Task creation failed\n");
        return 0;
    }
    
    result = xTaskCreate(print_task, "print", DEFAULT_STACK_SIZE, NULL, 2, &hPrintTask);

    if (result != pdPASS) {
        printf("Print Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(morse_task, "morse_task", DEFAULT_STACK_SIZE, NULL, 2, &hMorseTask);

    if(result != pdPASS) {
        printf("Morse Task creation failed\n");
        return 0;
    }

    tusb_init();
    usb_serial_init();

    printf("here");

    vTaskStartScheduler();
    
    return 0;
}

