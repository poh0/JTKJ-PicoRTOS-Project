#include "gesture.h"

#include <stdlib.h>

#include "imu.h"


#define NO_MOVEMENT_THRESHOLD 40        //deg/s
#define START_GESTURE_THRESHOLD 200     //deg/s
#define REVERSE_THRESHOLD 10            //deg/s
#define GESTURE_TIMEOUT_THRESHOLD 500   //ms
#define DELAY_BETWEEN_GESTURES 100      //ms
#define REGISTER_SPACE_DELAY 500        //ms
#define MIN_GESTURE_LENGTH_MS 100       //ms
#define MIN_RETURN_ANGLE_THRESHOLD 25000//deg/s * ms (25 degrees of tolerance)
#define TIME_BETWEEN_SAMPLES 10         //ms

typedef enum {
    STATE_MOVING_IDLE = 0, // no movement
    STATE_MOVING_OUT,      // started rotating the device outwards
    STATE_MOVING_IN        // started rotating the device inwards
} gesture_state_t;

//whole task needs some rewriting and cleanup and comments
void gesture_process_task(void *arg){
    //as we need access to 2 queues (consume imu que and produce to gesture que), pass a struct as pointer that includes the queue handles
    gesture_task_config_t *cfg = (gesture_task_config_t *)arg;
    QueueHandle_t imu_queue = cfg->imu_queue;           
    QueueHandle_t gesture_queue = cfg->gesture_queue;
    
    imu_data_t d;                       //raw imu data
    dir_t dir;                          //keeps track of the current direction

    gest_t last_gest = GEST_PAUSE;      //last logged gesture
    gesture_state_t gesture_state = STATE_MOVING_IDLE;

    uint32_t last_gesture_logged = 0;   //timestamp of last gesture logged   
    uint32_t gesture_timeout = 0;       //timestamp of when gesture towards current direction was started
    uint32_t last_measurement = 0;      //timestamp of last measurement
    int32_t int_angle = 0;              //integrated angle
    uint32_t gesture_start_time = 0;

    while(1){
        // Task is suspended by the scheduler here until an item is sent
        // Imutask sends imu_data_t at 100Hz when recording
        if (xQueueReceive(imu_queue, &d, portMAX_DELAY) == pdTRUE) {
            gyro_data_t gd = d.gyro;
            uint32_t cur_time = d.timestamp;//get the timestamp from the measurement itself
           
            int32_t gy = (int32_t)gd.y; //angular momentum is max +-2000 so fits in an int32, faster if no float arithmetic

            //in case of stale packet (timestamp == 0), don't handle packet
            if (cur_time <= last_measurement) continue;

            //integrate before switch to avoid redundancy:
            //integrate movement:
            //add to total current angular velocity multiplied by delta time
            int32_t dt = TIME_BETWEEN_SAMPLES;
            int_angle += (int32_t)gy * dt;

            //handle the movement:
            switch(gesture_state){
                //3 possible gestures: left twitch, right twitch and pause
                case STATE_MOVING_IDLE:
                    
                    //only if DELAY_BETWEEN_GESTURES from last gesture, advance
                    if(cur_time - last_gesture_logged < DELAY_BETWEEN_GESTURES) break;

                    //handle pause gesture
                    if(cur_time - last_gesture_logged > REGISTER_SPACE_DELAY){
                        //if first pause between string of morse, send a space
                        if(last_gest != GEST_PAUSE){
                            last_gest = GEST_PAUSE;
                            xQueueSend(gesture_queue, &last_gest, 0); 
                        }
                    }

                    //handle starting new twitch
                    if(abs(gy) >= START_GESTURE_THRESHOLD){
                        gesture_state = STATE_MOVING_OUT;
                        //log direction
                        dir = (gy > 0) ? RIGHT : LEFT; //right pos, left neg
                        
                        //integrate
                        int32_t dt = TIME_BETWEEN_SAMPLES;
                        int_angle = (int32_t)gy * dt; //set the total, dont add to total

                        //log time for timeout
                        gesture_timeout = cur_time;
                        gesture_start_time = cur_time;
                    }

                    break;
                case STATE_MOVING_OUT:
                    if (cur_time - gesture_timeout >= GESTURE_TIMEOUT_THRESHOLD) {
                        gesture_state = STATE_MOVING_IDLE;
                    }
                    else {
                        //handle if movement is reversed:
                        if ((dir == RIGHT && gy < -REVERSE_THRESHOLD) || (dir == LEFT && gy > REVERSE_THRESHOLD)) {
                            gesture_state = STATE_MOVING_IN;
                            gesture_timeout = cur_time;
                        }
                    }

                    break;
                case STATE_MOVING_IN:
                    //handle timeout: if movement not finished before threshold, discard gesture
                    if (cur_time - gesture_timeout >= GESTURE_TIMEOUT_THRESHOLD) {
                        gesture_state = STATE_MOVING_IDLE;
                    }

                    //handle if movement is stopped: gesture is completed
                    //only advance here, if 1. movement is under the no movement threshold, 
                    //2. integrated angle is within MIN_RETURN_ANGLE_THRESHOLD/1000 degrees of starting angle
                    //3. gesture has taken more than MIN_GESTURE_LENGTH_MS ms
                    else if (abs(gy) < NO_MOVEMENT_THRESHOLD && abs(int_angle) <= MIN_RETURN_ANGLE_THRESHOLD && cur_time - gesture_start_time > MIN_GESTURE_LENGTH_MS){
                        last_gest = dir;
                        last_gesture_logged = cur_time;
                        gesture_state = STATE_MOVING_IDLE;
                        
                        //send the direction to gesture queue
                        xQueueSend(gesture_queue, &last_gest, 0);                
                    }
                    break;
            }
            //log last measurement timestamp
            last_measurement = cur_time;
        }
    }
}