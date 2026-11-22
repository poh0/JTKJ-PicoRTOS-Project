#pragma once

#include <FreeRTOS.h>
#include <queue.h>

typedef enum {
    RIGHT = 0,
    LEFT
} dir_t;

typedef enum { //gest_t = dir_t should be a correct assignment
    GEST_RIGHT = 0,
    GEST_LEFT,
    GEST_PAUSE
} gest_t;

// Suggested by AI, when asked how do I pass both queues as parameter to the task
typedef struct {
    QueueHandle_t imu_queue;
    QueueHandle_t gesture_queue;
} gesture_task_config_t;

void gesture_process_task(void *arg);
