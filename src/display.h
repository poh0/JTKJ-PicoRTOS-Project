#pragma once

#include "tkjhat/ssd1306.h"

typedef struct {
    char symbol;
    bool is_rx;
} display_data_t;

void display_task(void *arg);