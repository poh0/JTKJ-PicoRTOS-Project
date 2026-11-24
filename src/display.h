#pragma once

#include "tkjhat/ssd1306.h"

#include <semphr.h>

// AI prompt: How do I notify display task from morse task to reset
// AI suggested to use a command enum, I modified it a bit
typedef enum {
    DISPLAY_CMD_SYMBOL_TX,
    DISPLAY_CMD_SYMBOL_RX,
    DISPLAY_CMD_RESET
} display_cmd_t;

typedef struct {
    display_cmd_t cmd;
    char symbol;
} display_data_t;

extern SemaphoreHandle_t i2c_mutex;

void display_task(void *arg);

void nt_array(char * arr, uint8_t index);