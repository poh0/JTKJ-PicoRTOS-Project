#include <FreeRTOS.h>
#include <queue.h>
#include <string.h>

#include "morse.h"
#include "display.h"
#include "tkjhat/sdk.h"
#include "sound.h"

void display_task(void *arg){
    QueueHandle_t *display_queue = (QueueHandle_t *)arg;
    display_data_t d;

    uint8_t tx_len = 0;
    uint8_t rx_len = 0;

    uint8_t cx = 0;
    uint8_t cy = 0;

    //work with 2 arrays: morse array and letter arrat
    uint8_t ma_len = 0;
    char morse_array[17] = ""; //fits 16 chars and null termination

    uint8_t la_len = 0;
    char letter_array[17] = "";

    while (1) {
        if (xQueueReceive(*display_queue, &d, portMAX_DELAY) == pdTRUE) {
            
            if (!d.is_rx) {
                if(d.symbol != ' '){
                    if(ma_len < 16){
                        morse_array[ma_len] = d.symbol;
                        morse_array[ma_len + 1] = '\0'; //always make sure string is null terminated
                        ma_len++;
                    }
                }
                else{//if encounter a space:
                    char letter = lookup_morse(morse_array);
                    letter_array[la_len] = letter;
                    la_len++;

                    //reset morse array (we null terminate anyway after every character, so resetting the index is enough)
                    ma_len = 0;
                }
            }

            if(xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE){
                //write morse and letter arrays in different heights
                clear_display();
                write_text_xy(cx, 0, morse_array);
                write_text_xy(cx, 10, letter_array);
                xSemaphoreGive(i2c_mutex);
            }
        }
    }
}
