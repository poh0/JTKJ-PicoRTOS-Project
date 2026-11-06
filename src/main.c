
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

#include <tusb.h>
#include "usbSerialDebug/helper.h"

#include "imu.h"

#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1

#define MAX_MORSE_LEN 64
#define IMU_QUEUE_LEN 20

typedef enum { 
    STATE_IDLE = 0,
    STATE_RECORDING,
    STATE_PROCESSING,
    STATE_SENDING,
    STATE_RECEIVING
} system_state_t;

system_state_t programState = STATE_IDLE;

// Buffer for morse characters
char morse_buf[MAX_MORSE_LEN];

QueueHandle_t imu_queue;

static void btn_fxn(uint gpio, uint32_t eventMask) {
    
}

static void imu_task(void *arg){
    (void)arg;
    // init imu ?
    imu_data_t raw_reading;

    for(;;){

        if (programState != STATE_RECORDING) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        imu_read_raw(&raw_reading);

        xQueueSend(imu_queue, &raw_reading, 0);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void print_task(void *arg){
    (void)arg;
    
    while(1){
        tight_loop_contents();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void usbTask(void *arg) {
    (void)arg;
    while (1) {
        tud_task();              // With FreeRTOS wait for events
    }
}

int main() {
    //stdio_init_all();

    // Uncomment this lines if you want to wait till the serial monitor is connected
    /*while (!stdio_usb_connected()){
        sleep_ms(10);
    }*/ 
    
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    // Initialize the button and the led and define an register the corresponding interrupton.
    init_sw1();
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, btn_fxn);
    init_red_led();

    imu_queue = xQueueCreate(20, sizeof(imu_data_t));

    TaskHandle_t hImuTask, hPrintTask, hUSB = NULL;

    xTaskCreate(usbTask, "usb", 2048, NULL, 3, &hUSB);
    #if (configNUMBER_OF_CORES > 1)
        vTaskCoreAffinitySet(hUSB, 1u << 0);
    #endif
    
    // Create the tasks with xTaskCreate
    BaseType_t result = xTaskCreate(imu_task, // (en) Task function
                "imu",                        // (en) Name of the task 
                DEFAULT_STACK_SIZE,              // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                            // (en) Arguments of the task 
                2,                               // (en) Priority of this task
                &hImuTask);                   // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("IMU task creation failed\n");
        return 0;
    }
    
    result = xTaskCreate(print_task,  // (en) Task function
                "print",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hPrintTask);         // (en) A handle to control the execution of this task

    if (result != pdPASS) {
        printf("Print Task creation failed\n");
        return 0;
    }

    tusb_init();
    usb_serial_init();

    // Start the scheduler (never returns)
    vTaskStartScheduler();
    
    return 0;
}

