#include <FreeRTOS.h>
#include "display.h"
#include "tkjhat/sdk.h"

void display_task(void *arg){
    QueueHandle_t display_queue = arg; //mutulla nakattu
    display_data_t d;
    char strbf[16] = "";
    uint8_t cx = 0; //cursor position
    uint8_t cy = 0;
    char str[] = "asdfdsasdf";
    write_text_xy(100, 100, str);
    while(1){
        if (xQueueReceive(display_queue, &d, portMAX_DELAY) == pdTRUE) {
            char c[2] = {d.symbol, '\0'};
            //is not recieved: means this is our self recorded data to be displayed
            if(!d.is_rx){
                write_text_xy(cx + 10, cy, c);
            }
            
        }
        //display does not need frequent updating
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}