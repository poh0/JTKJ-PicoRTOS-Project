#include <FreeRTOS.h>
#include <queue.h>
#include <string.h>

#include "morse.h"
#include "display.h"
#include "tkjhat/sdk.h"
#include "ringbuffer.h"

#define MAX_MORSE_SIZE 16
#define LETTER_ARRAY_SIZE 30
#define LETTERS_PER_ROW 20
#define PIXELS_PER_LINE 9

void display_task(void *arg){
    QueueHandle_t *display_queue = (QueueHandle_t *)arg;
    display_data_t d;

    uint8_t tx_len = 0;
    uint8_t rx_len = 0;

    uint8_t cx = 0;
    uint8_t cy = 0;

    //work with 2 data structures per direction: morse array and letter ringbuffer tx and rx
    uint8_t ma_tx_len = 0;
    char morse_tx_array[MAX_MORSE_SIZE + 1] = ""; //fits MAX_MORSE_SIZE chars and null termination (longest morse translation is 5 chars anyway)

    uint8_t la_tx_len = 0;
    ring_buf_t tx_letter_buf = {0};

    uint8_t ma_rx_len = 0;
    char morse_rx_array[MAX_MORSE_SIZE + 1] = "";

    uint8_t la_rx_len = 0;
    ring_buf_t rx_letter_buf = {0};

    while (1) {
        if (xQueueReceive(*display_queue, &d, portMAX_DELAY) == pdTRUE) {
            
            if (d.cmd == DISPLAY_CMD_RESET) {
                ma_tx_len = 0;
                morse_tx_array[0] = '\0';
                tx_letter_buf = (ring_buf_t){0};
                if(xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE){
                    write_text_xy(0,  0*PIXELS_PER_LINE, "Sent! ");
                    xSemaphoreGive(i2c_mutex);
                }
                continue; // go back to loop start
            }
            //handle TX morse translation
            if (d.cmd == DISPLAY_CMD_SYMBOL_TX) {
                if(d.symbol != ' '){
                    if(ma_rx_len < MAX_MORSE_SIZE){
                        
                        morse_tx_array[ma_tx_len] = d.symbol;
                        morse_tx_array[ma_tx_len + 1] = '\0'; //always make sure string is null terminated
                        ma_tx_len++;                        
                    }
                }
                else {//if encounter a space:
                    char letter;
                    if (ma_tx_len == 0){
                        letter = ' ';
                    }
                    else{
                        letter = lookup_morse(morse_tx_array);
                    }
                    if(la_tx_len < LETTER_ARRAY_SIZE){
                        rb_push(&tx_letter_buf, letter);
                    }

                    //reset morse array
                    ma_tx_len = 0;
                    morse_tx_array[0] = '\0';
                }
            }
            //redundant code here...
            //handle RX morse translation
            if (d.cmd == DISPLAY_CMD_SYMBOL_RX) {
                if(d.symbol != ' '){
                    if(ma_rx_len < MAX_MORSE_SIZE){
                        morse_rx_array[ma_rx_len] = d.symbol;
                        morse_rx_array[ma_rx_len + 1] = '\0'; //always make sure string is null terminated
                        ma_rx_len++;
                    }
                }
                else {//if encounter a space:
                    char letter;
                    if (ma_rx_len == 0){
                        letter = ' ';
                    }
                    else{
                        letter = lookup_morse(morse_rx_array);
                    }
                    if(la_rx_len < LETTER_ARRAY_SIZE){
                        rb_push(&rx_letter_buf, letter);
                    }
                    //reset morse array
                    ma_rx_len = 0;
                    morse_rx_array[0] = '\0';
                }
            }

            if(xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE){
                //write morse and letter arrays in different heights
                clear_display();

                write_text_xy(cx, PIXELS_PER_LINE, morse_tx_array);

                char text[BUF_SIZE + 1];

                write_text_xy(0, 4 * PIXELS_PER_LINE, "Recieved: ");
                write_text_xy(0, 5 * PIXELS_PER_LINE, morse_rx_array);

                rb_to_string(&tx_letter_buf, text, sizeof(text));
                write_text_xy(cx, 2*PIXELS_PER_LINE, text); //2nd row: write ringbuffer

                rb_to_string(&rx_letter_buf, text, sizeof(text));
                write_text_xy(cx, 6 * PIXELS_PER_LINE, text); //6th row: write ringbuffer
                xSemaphoreGive(i2c_mutex);
            }
        }
    }
}
