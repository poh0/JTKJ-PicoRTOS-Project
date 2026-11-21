#include "gesture.h"

#include <stdio.h>
#include <stdlib.h>
#include <tusb.h>
#include "imu.h"

#define ANG_SPEED_THRESHOLD 200
#define NO_MOVEMENT_THRESHOLD 40
#define GESTURE_TIMEOUT_THRESHOLD 500
#define DELAY_BETWEEN_GESTURES 300

typedef enum {
    STATE_MOVING_IDLE = 0, // no movement
    STATE_MOVING_OUT,      // started rotating the device outwards
    STATE_MOVING_IN        // started rotating the device inwards
} gesture_state_t;

static gesture_state_t gesture_state = STATE_MOVING_IDLE;

void gesture_process_task(void *arg){
    
    gesture_task_config_t *cfg = (gesture_task_config_t *)arg;
    QueueHandle_t imu_queue = cfg->imu_queue;
    QueueHandle_t gesture_queue = cfg->gesture_queue;
    
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

            int16_t gy = (int16_t)gd.y; //angular momentum is max +-250 so fits in an int16, faster if no float arithmetic
            uint32_t cur_time = t; //get the timestamp from the measurement itself

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
                    gesture_state = STATE_MOVING_IDLE;
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
                    
                    //send the direction to gesture queue
                    xQueueSend(gesture_queue, &last_gesture, 0);                
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