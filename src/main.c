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
#include "music.h"

#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1

#define MAX_MORSE_LEN 64
#define IMU_QUEUE_LEN 20

#define ANG_SPEED_THRESHOLD 200
#define NO_MOVEMENT_THRESHOLD 40

#define GESTURE_TIMEOUT_THRESHOLD 500
#define DELAY_BETWEEN_GESTURES 300


typedef enum { 
    STATE_IDLE = 0,
    STATE_RECORDING,
    STATE_PROCESSING,
    STATE_SENDING,
    STATE_RECEIVING
} system_state_t;

typedef enum {
    STATE_MOVING_IDLE = 0, //no movement
    STATE_MOVING_OUT,      //started rotating the device outwards
    STATE_MOVING_IN,       //started rotating the device inwards
} gesture_state_t;

typedef enum {
    RIGHT = 0,
    LEFT
} dir_t;

system_state_t program_state = STATE_IDLE;
gesture_state_t gesture_state = STATE_MOVING_IDLE;

// Buffer for morse characters
char morse_buf[MAX_MORSE_LEN];

QueueHandle_t imu_queue;
QueueHandle_t sound_queue;

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

static void gesture_process_task(void *arg){
    (void)arg;
    
    imu_data_t d;
    dir_t dir; //keeps track of the current direction
    dir_t last_gesture = 0;
    uint32_t last_gesture_logged = 0;
    uint32_t gesture_timeout = 0;
    uint32_t last_measurement = 0;
    int32_t int_angle = 0;

    while(1){
        // Task is suspended by the scheduler here until an item is sent
        if (xQueueReceive(imu_queue, &d, portMAX_DELAY) == pdTRUE) {
            acc_data_t ad = d.acc;
            gyro_data_t gd = d.gyro;
            uint32_t t = d.timestamp;

            //printf("Gyro: (%f, %f, %f)\nAccl: (%f, %f, %f)\nTime:%ld\n", gd.x,gd.y,gd.z, ad.x, ad.y, ad.z, t);
            //using tinyusb, formatting by gemini:

            char buf[128];
            //int len = snprintf(buf, sizeof(buf), "Gyro: (%f, %f, %f)\nAccl: (%f, %f, %f)\nTime:%ld\n", gd.x, gd.y, gd.z, ad.x, ad.y, ad.z, t);
            int len = snprintf(buf, sizeof(buf), "Integral: %ld, Last gesture: %d, gesture state: %d gy: %f, time: %ld\n",int_angle, last_gesture,gesture_state, gd.y,  t);
            if (len > 0) {
                // Ensure we don't try to write more than the buffer size if truncated
                uint32_t write_len = (len < sizeof(buf)) ? len : (sizeof(buf) - 1);
                
                tud_cdc_write(buf, write_len);
                tud_cdc_write_flush(); // Forces the USB packet to be sent immediately
            }

            int16_t gy = (int16_t)gd.y;//angular momentum is max +-250 so fits in an int16, faster if no float arithmetic
            uint32_t cur_time = t;//get the timestamp from the measurement itself

            //handle the movement:
            switch(gesture_state){
            case STATE_MOVING_IDLE:
                //only if DELAY_BETWEEN_GESTURES from last gesture, advance
                if(cur_time - last_gesture_logged < DELAY_BETWEEN_GESTURES){
                    break;
                }
                if(abs(gy) >= ANG_SPEED_THRESHOLD){
                    gesture_state = STATE_MOVING_OUT;
                    //log direction
                    dir = (gy > 0) ? RIGHT : LEFT; //right pos, left neg
                    //log time for timeout
                    gesture_timeout = cur_time;
                    last_measurement = cur_time;
                    int_angle = 0;
                }
                break;
            case STATE_MOVING_OUT:
                //integrate movement:
                //add to total current angular velocity multiplied by delta time
                int_angle += (gy * (cur_time - last_measurement));

                //handle if movement is reversed:
                if(dir == RIGHT){
                    if(gy < 0){
                        gesture_state = STATE_MOVING_IN;
                        //also we need to set the timeout
                        gesture_timeout = cur_time;
                    }
                }
                else{
                    if(gy > 0){
                        gesture_state = STATE_MOVING_IN;
                        gesture_timeout = cur_time;
                    }
                }

                //handle timeout: if movement not reversed before threshold, put into idle state
                if(cur_time - gesture_timeout >= GESTURE_TIMEOUT_THRESHOLD){
                    gesture_state = STATE_IDLE;
                }

                //log last measurement timestamp
                last_measurement = cur_time;
                break;
            case STATE_MOVING_IN:
                //integrate movement:
                //add to total current angular velocity multiplied by delta time
                int_angle += (gy * (cur_time - last_measurement));

                //handle if movement is stopped:
                if(gy < NO_MOVEMENT_THRESHOLD && int_angle <= 15000 && cur_time - gesture_timeout > 100){ //15000 because we multiplied by dMS which is 1000* dS
                    //also added a timeout of 100ms before we can advace to here
                    last_gesture = dir;
                    last_gesture_logged = cur_time;
                    gesture_state = STATE_MOVING_IDLE;
                    //send the direction to sound que
                    xQueueSend(sound_queue, &last_gesture, 0);
                }

                //handle timeout: if movement not reversed before threshold, put into idle state
                if(cur_time - gesture_timeout >= GESTURE_TIMEOUT_THRESHOLD){
                    gesture_state = STATE_MOVING_IDLE;
                }

                //log last measurement timestamp
                last_measurement = cur_time;
                break;
            }
        }
    }
}

//plays the morse dot or line gestured
static void buzzer_task(void *arg) {
    (void)arg;
    dir_t dir;

    note dot = {NOTE_C5, LEN_SIXTEENTH};
    note line = {NOTE_C5, LEN_QUARTER};
    while(1){
        // Task is suspended by the scheduler here until an item is sent
        if (xQueueReceive(sound_queue, &dir, portMAX_DELAY) == pdTRUE) {
            if (dir == LEFT) {
                play_note(&line);
            } else if (dir == RIGHT) {
                play_note(&dot);
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

    imu_queue = xQueueCreate(20, sizeof(imu_data_t));

    sound_queue = xQueueCreate(5, sizeof(dir_t));

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
    
    result = xTaskCreate(gesture_process_task, "gesture_processing", DEFAULT_STACK_SIZE, NULL, 2, &hGestureProcessTask);

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

