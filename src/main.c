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

#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1

#define MAX_MORSE_LEN 64
#define IMU_QUEUE_LEN 20

typedef enum { 
    STATE_IDLE = 0,
    STATE_RECORDING,
    STATE_PROCESSING,
    STATE_SENDING,
    STATE_RECEIVING
} system_state_t;

system_state_t program_state = STATE_IDLE;

// Buffer for morse characters
char morse_buf[MAX_MORSE_LEN];

// Producer: Imu task
// Consumer: Gesture processing task
QueueHandle_t imu_queue;

// Producer: Gesture processing task
// Consumer: state task
QueueHandle_t gesture_queue;

// producer: State task
// Consumer: Buzzer task
QueueHandle_t sound_queue;

// producer: State task
// consumer: display task
QueueHandle_t display_queue;

static void btn_fxn(uint gpio, uint32_t eventMask) {
    
}

static void imu_task(void *arg){
    (void)arg;

    imu_data_t raw_reading = {0};

    for(;;){

        imu_read_raw(&raw_reading);
        xQueueSend(imu_queue, &raw_reading, 0);

        vTaskDelay(pdMS_TO_TICKS(10));
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
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, btn_fxn);
    init_red_led();

    init_imu();
    start_imu();

    imu_queue     = xQueueCreate(20, sizeof(imu_data_t));
    gesture_queue = xQueueCreate(10, sizeof(dir_t));
    sound_queue   = xQueueCreate(5,  sizeof(sound_event_t));
    display_queue = xQueueCreate(16, sizeof(char));

    TaskHandle_t hImuTask, hPrintTask, hUSB, hBuzzerTask, hGestureProcessTask = NULL;

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
        printf("Buzzer Task created failed\n");
        return 0;
    }

    result = xTaskCreate(buzzer_task, "buzzer", 1024, NULL, 2, &hBuzzerTask);

    if(result != pdPASS) {
        printf("Buzzer Task created failed\n");
        return 0;
    }
    
    result = xTaskCreate(print_task, "print", DEFAULT_STACK_SIZE, NULL, 2, &hPrintTask);

    if (result != pdPASS) {
        printf("Print Task creation failed\n");
        return 0;
    }

    tusb_init();
    usb_serial_init();

    vTaskStartScheduler();
    
    return 0;
}

