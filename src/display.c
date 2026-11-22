#include <FreeRTOS.h>
#include <queue.h>
#include <string.h>

#include "display.h"
#include "tkjhat/sdk.h"


void display_task(void *arg){

    QueueHandle_t *display_queue = (QueueHandle_t *)arg;
    display_data_t d;

    uint8_t tx_len = 0;
    uint8_t rx_len = 0;

    while (1) {
        if (xQueueReceive(*display_queue, &d, portMAX_DELAY) == pdTRUE) {
            if (!d.is_rx) {
                
            }
        }
    }
}