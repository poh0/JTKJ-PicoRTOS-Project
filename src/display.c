#include <FreeRTOS.h>
#include <queue.h>
#include <string.h>

#include "morse.h"
#include "display.h"
#include "tkjhat/sdk.h"

#define MORSE_ARRAY_SIZE 16
#define LETTER_ARRAY_SIZE 100
#define LETTERS_PER_ROW 20
#define PIXELS_PER_LINE 10

void display_task(void *arg){
    QueueHandle_t *display_queue = (QueueHandle_t *)arg;
    display_data_t d;

    uint8_t tx_len = 0;
    uint8_t rx_len = 0;

    uint8_t cx = 0;
    uint8_t cy = 0;

    //work with 2 arrays: morse array and letter arrat
    uint8_t ma_len = 0;
    char morse_array[MORSE_ARRAY_SIZE + 1] = ""; //fits MORSE_ARRAY_SIZE chars and null termination (longest morse translation is 5 chars anyway)

    uint8_t la_len = 0;
    char letter_array[LETTER_ARRAY_SIZE + 1] = "";

    while (1) {
        if (xQueueReceive(*display_queue, &d, portMAX_DELAY) == pdTRUE) {
            
            if (d.cmd == DISPLAY_CMD_RESET) {
                ma_len = 0;
                la_len = 0;
                morse_array[0] = '\0';
                letter_array[0] = '\0';

                if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
                    clear_display();
                    xSemaphoreGive(i2c_mutex);
                }
                continue; // go back to loop start
            }

            if (d.cmd == DISPLAY_CMD_SYMBOL_TX) {
                if(d.symbol != ' '){
                    if(ma_len < MORSE_ARRAY_SIZE){
                        morse_array[ma_len] = d.symbol;
                        morse_array[ma_len + 1] = '\0'; //always make sure string is null terminated
                        ma_len++;
                    }
                }
                else {//if encounter a space:
                    if (ma_len == 0) continue;

                    char letter = lookup_morse(morse_array);
                    if(la_len < LETTER_ARRAY_SIZE){
                        letter_array[la_len] = letter;
                        la_len++;
                    }

                    //reset morse array
                    ma_len = 0;
                    morse_array[0] = '\0';
                }
            }

            if(xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE){
                //write morse and letter arrays in different heights
                clear_display();
                write_text_xy(cx, 0, morse_array);
                // 20 letters in row
                char temp[LETTERS_PER_ROW + 1];
                int total_rows = (la_len + LETTERS_PER_ROW - 1) / LETTERS_PER_ROW; // Rounds up
                //from gemini, prompt was broken version of same code iterating over 2 for loops
                //this version uses memcpy and is more efficient and safe doing the same thing
                //the overhead this adds is some microseconds, which is nothing compared to the i2c transactions...
                for (int i = 0; i < total_rows; i++) {
                    int offset = i * LETTERS_PER_ROW;
                    int chars_to_copy = la_len - offset;
                    
                    if (chars_to_copy > LETTERS_PER_ROW) {
                        chars_to_copy = LETTERS_PER_ROW;
                    }

                    memcpy(temp, &letter_array[offset], chars_to_copy);
                    temp[chars_to_copy] = '\0'; // Ensure null termination for partial rows

                    write_text_xy(cx, PIXELS_PER_LINE + (i * PIXELS_PER_LINE), temp);
                }
                
                xSemaphoreGive(i2c_mutex);
            }
        }
    }
}
