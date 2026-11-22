#pragma once

#include "tkjhat/ssd1306.h"

#include <semphr.h>

typedef struct {
    char symbol;
    bool is_rx;
} display_data_t;

extern SemaphoreHandle_t i2c_mutex;

void display_task(void *arg);