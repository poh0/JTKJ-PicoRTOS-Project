//from gemini, prompt "make an example for the buzzer.h library", some edits made
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"
#include "tkjhat/buzzer.h"

#ifndef BUZZER_PIN
#define BUZZER_PIN 17
#endif

// Persistent melody array using the note_freq_t and note_length_t enums
static const buzzer_note_t demo_melody[] = {
    { NOTE_E5,   LEN_EIGHTH },
    { NOTE_E5,   LEN_EIGHTH },
    { NOTE_REST, LEN_EIGHTH },
    { NOTE_E5,   LEN_EIGHTH },
    { NOTE_REST, LEN_EIGHTH },
    { NOTE_C5,   LEN_EIGHTH },
    { NOTE_E5,   LEN_QUARTER },
    { NOTE_G5,   LEN_QUARTER },
    { NOTE_REST, LEN_QUARTER },
    { NOTE_G4,   LEN_HALF }
};

static void leds_task(void *arg) {
    (void)arg;

    // Initialize LED
    init_led();
    printf("Initializing on board led\n");

    while (1) {
        toggle_led();
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

static void display_task(void *arg) {
    (void)arg;

    init_display();
    printf("Initializing display\n");
    static int counter = 0;

    while (1) {
        clear_display();
        char buf[12];
        sprintf(buf, "%d", counter++);
        write_text(buf);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void buzzer_task(void *arg) {
    (void)arg;

    // Initialize the non-blocking PWM + FreeRTOS timer buzzer driver
    buzzer_init(BUZZER_PIN);
    buzzer_set_volume(2);
    printf("Initializing non-blocking buzzer on GPIO at 2% volume %d\n", BUZZER_PIN);

    // 1. Play a single asynchronous startup tone (returns immediately)
    buzzer_tone_async(NOTE_A5, LEN_EIGHTH);
    printf("Startup tone triggered (non-blocking)\n");
    vTaskDelay(pdMS_TO_TICKS(1500));

    const size_t melody_len = sizeof(demo_melody) / sizeof(demo_melody[0]);

    while (1) {
        // 2. Play the full melody in the background while LED and OLED tasks continue running
        printf("Playing full melody asynchronously...\n");
        buzzer_play_melody_async(demo_melody, melody_len);
        vTaskDelay(pdMS_TO_TICKS(4500));

        // 3. Demonstrate interrupting playback mid-melody with buzzer_stop()
        printf("Starting melody again and stopping early after 1 second...\n");
        buzzer_play_melody_async(demo_melody, melody_len);
        vTaskDelay(pdMS_TO_TICKS(1000));
        buzzer_stop();
        printf("Buzzer stopped.\n");

        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}

int main() {
    stdio_init_all();

    // Wait until the USB serial monitor is connected before continuing.
    // Comment out this loop if you want the board to run standalone without USB serial.
    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }

    init_hat_sdk();
    sleep_ms(400);

    TaskHandle_t ledsTaskHandle = NULL;
    TaskHandle_t displayTaskHandle = NULL;
    TaskHandle_t buzzerTaskHandle = NULL;

    BaseType_t result = xTaskCreate(leds_task,
                                    "led",
                                    1024,
                                    NULL,
                                    2,
                                    &ledsTaskHandle);
    if (result != pdPASS) {
        printf("Led Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(buzzer_task,
                         "buzzer",
                         1024,
                         NULL,
                         2,
                         &buzzerTaskHandle);
    if (result != pdPASS) {
        printf("Buzzer Task creation failed\n");
        return 0;
    }

    result = xTaskCreate(display_task,
                         "display",
                         1024,
                         NULL,
                         2,
                         &displayTaskHandle);
    if (result != pdPASS) {
        printf("Display Task creation failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    vTaskStartScheduler();

    return 0;
}