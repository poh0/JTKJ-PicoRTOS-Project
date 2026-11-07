
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"
#define NOTE_DELAY 10
//by gemini AI, prompt: "create an c array of note frequencies for buzzer range, and also create an enum that maps the array of frequencies to notes (such as A5 or C#5) "
// Enum mapping note names to array indices
typedef enum {
    NOTE_REST = 0, // 0 Hz (Silence)
    
    // Octave 4
    NOTE_C4,
    NOTE_CS4, // C# / Db
    NOTE_D4,
    NOTE_DS4, // D# / Eb
    NOTE_E4,
    NOTE_F4,
    NOTE_FS4, // F# / Gb
    NOTE_G4,
    NOTE_GS4, // G# / Ab
    NOTE_A4,
    NOTE_AS4, // A# / Bb
    NOTE_B4,

    // Octave 5
    NOTE_C5,
    NOTE_CS5,
    NOTE_D5,
    NOTE_DS5,
    NOTE_E5,
    NOTE_F5,
    NOTE_FS5,
    NOTE_G5,
    NOTE_GS5,
    NOTE_A5,
    NOTE_AS5,
    NOTE_B5,

    // Octave 6
    NOTE_C6,
    NOTE_CS6,
    NOTE_D6,
    NOTE_DS6,
    NOTE_E6,
    NOTE_F6,
    NOTE_FS6,
    NOTE_G6,
    NOTE_GS6,
    NOTE_A6,
    NOTE_AS6,
    NOTE_B6,
    NOTE_END, //to end song
    // Total number of notes mapped
    NUM_BUZZER_NOTES
} buzzer_note;

// Array of frequencies matching the enum indices above
// Using 'unsigned int' as frequencies are always positive
const unsigned int buzzer_frequencies[NUM_BUZZER_NOTES] = {
    0,    // NOTE_REST

    // Octave 4
    262,  // NOTE_C4
    277,  // NOTE_CS4
    294,  // NOTE_D4
    311,  // NOTE_DS4
    330,  // NOTE_E4
    349,  // NOTE_F4
    370,  // NOTE_FS4
    392,  // NOTE_G4
    415,  // NOTE_GS4
    440,  // NOTE_A4
    466,  // NOTE_AS4
    494,  // NOTE_B4

    // Octave 5
    523,  // NOTE_C5
    554,  // NOTE_CS5
    587,  // NOTE_D5
    622,  // NOTE_DS5
    659,  // NOTE_E5
    698,  // NOTE_F5
    740,  // NOTE_FS5
    784,  // NOTE_G5
    831,  // NOTE_GS5
    880,  // NOTE_A5
    932,  // NOTE_AS5
    988,  // NOTE_B5

    // Octave 6
    1047, // NOTE_C6
    1109, // NOTE_CS6
    1175, // NOTE_D6
    1245, // NOTE_DS6
    1319, // NOTE_E6
    1397, // NOTE_F6
    1480, // NOTE_FS6
    1568, // NOTE_G6
    1661, // NOTE_GS6
    1760, // NOTE_A6
    1865, // NOTE_AS6
    1976  // NOTE_B6
};

typedef struct{
    buzzer_note tone;
    note_length length;
}note;

//also generated with gemini
// Enum mapping note durations to array indices
typedef enum {
    LEN_WHOLE = 0,           // Whole note (1)
    LEN_DOTTED_HALF,         // Dotted half note (1.5x half)
    LEN_HALF,                // Half note (1/2)
    LEN_DOTTED_QUARTER,      // Dotted quarter note
    LEN_QUARTER,             // Quarter note (1/4) - 1 beat at 120 BPM
    LEN_DOTTED_EIGHTH,       // Dotted eighth note
    LEN_EIGHTH,              // Eighth note (1/8)
    LEN_DOTTED_SIXTEENTH,    // Dotted sixteenth note
    LEN_SIXTEENTH,           // Sixteenth note (1/16)
    
    // Total number of lengths mapped
    NUM_NOTE_LENGTHS
} note_length;

// Array of durations in milliseconds (calculated for 120 BPM)
// Using 'unsigned int' (or uint16_t) as max value is 2000
const unsigned int note_durations_ms[NUM_NOTE_LENGTHS] = {
    2000, // LEN_WHOLE 
    1500, // LEN_DOTTED_HALF 
    1000, // LEN_HALF 
    750,  // LEN_DOTTED_QUARTER
    500,  // LEN_QUARTER (1 beat at 120 BPM = 60,000ms / 120 = 500ms)
    375,  // LEN_DOTTED_EIGHTH
    250,  // LEN_EIGHTH
    187,  // LEN_DOTTED_SIXTEENTH (rounded from 187.5)
    125   // LEN_SIXTEENTH
};


note sik_hymn[] = { //C and F sharps
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_E5, LEN_EIGHTH},
    {NOTE_FS5, LEN_QUARTER},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_FS5, LEN_QUARTER},
    {NOTE_E5, LEN_QUARTER},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_CS5, LEN_DOTTED_QUARTER},
    {NOTE_CS5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_B4, LEN_EIGHTH},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_E5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_HALF},
    {NOTE_REST, LEN_QUARTER},//1st row

    {NOTE_A4, LEN_EIGHTH},
    {NOTE_A4, LEN_EIGHTH},
    {NOTE_FS5, LEN_QUARTER},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_E5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_CS5, LEN_DOTTED_QUARTER},
    {NOTE_B4, LEN_QUARTER},
    {NOTE_B4, LEN_EIGHTH},
    {NOTE_CS5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_E5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_CS5, LEN_QUARTER},
    {NOTE_A4, LEN_QUARTER},
    {NOTE_B4, LEN_HALF},
    {NOTE_REST, LEN_QUARTER},
    {NOTE_END, LEN_DOTTED_EIGHTH}
};

//const int sik_hymn_length = sizeof(sik_hymn) / sizeof(sik_hymn[0]);

void play_note(note * n){
    if(n->tone != NOTE_REST){
        //lookup the corresponding values for note freq and lenght
        buzzer_play_tone(buzzer_frequencies[n->tone], note_durations_ms[n->length] - NOTE_DELAY); //NOTE_DELAY ms of silence after each note so we can differentiate long sequences of same note, doesnt mess with timing
        buzzer_turn_off();
        vTaskDelay(pdMS_TO_TICKS(NOTE_DELAY));//wait out the rest of the note
    }
    else{
        //if note is rest, wait out the whole note
        vTaskDelay(pdMS_TO_TICKS(note_durations_ms[n->length]));
    }
}

void play_song(note * song){
    note * cur_note = song;
    while(cur_note->tone != NOTE_END){
        play_note(cur_note);
        cur_note++; //traverse pointer
    }
}

static void leds_task(void *arg) {
    (void)arg;

    // Initialize LED
    init_led();
    printf("Initializing on board led\n");

    while (1) {
        toggle_led();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void display_task(void *arg) {
    (void)arg;

    init_display();
    printf("Initializing display\n");
    static int counter = 0; 

    while(1) {
        
        clear_display();
        char buf[5]; //Store a number of maximum 5 figures 
        sprintf(buf,"%d",counter++);
        write_text(buf);
        vTaskDelay(pdMS_TO_TICKS(4000));
    }

}

static void buzzer_task(void *arg) {
    (void)arg;

    //Initialize the buzzer
    init_buzzer();
    printf("Initializing buzzer\n");
    while(1){
        play_song(sik_hymn);
    }

}


int main() {
    stdio_init_all();
    // Wait until the USB serial monitor is connected before continuing.
    // This example intentionally pauses here until a serial connection is opened.
    // You can comment the while, if you want the program to run without waiting for a serial connection (you might lose some debug output if you do this).
    //while (!stdio_usb_connected()){
   //     sleep_ms(10);
    //} 
    init_hat_sdk();
    sleep_ms(400); 

    TaskHandle_t ledsTaskHandle, displayTaskHandle, buzzerTaskHandle = NULL;
   
    // Create the single blinky/printf task
    // (en) We create a task
    BaseType_t result = xTaskCreate(leds_task,       // (en) Task function
                "led",              // (en) Name of the task 
                1024,               // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,               // (en) Arguments of the task 
                2,                  // (en) Priority of this task
                &ledsTaskHandle);   // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Led Task created failed\n");
        return 0;
    }

    result = xTaskCreate(buzzer_task,   // (en) Task function
            "buzzer",                   // (en) Name of the task 
                1024,                   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                   // (en) Arguments of the task 
                2,                      // (en) Priority of this task
                &buzzerTaskHandle);     // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Buzzer Task created failed\n");
        return 0;
    }
    result = xTaskCreate(display_task,   // (en) Task function
            "display",                   // (en) Name of the task 
                1024,                    // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                    // (en) Arguments of the task 
                2,                       // (en) Priority of this task
                &displayTaskHandle);     // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Display Task created failed\n");
        return 0;
    }
    // Start the scheduler (never returns)
    vTaskStartScheduler();



    
    return 0;
}

