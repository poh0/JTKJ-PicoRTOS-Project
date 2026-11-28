#include "sound.h"

#include <FreeRTOS.h>

#include <task.h>

#include "tkjhat/sdk.h"

void play_note(note_t * n){
    if(n->tone != NOTE_REST) {
        buzzer_play_tone(n->tone, n->length - NOTE_DELAY); //NOTE_DELAY ms of silence after each note so we can differentiate long sequences of same note, doesnt mess with timing
        buzzer_turn_off();
        vTaskDelay(pdMS_TO_TICKS(NOTE_DELAY));//wait out the rest of the note
    }
    else {
        //if note is rest, wait out the whole note
        vTaskDelay(pdMS_TO_TICKS(n->length));
    }
}

void play_song(note_t * song){
    note_t *cur_note = song;
    while(cur_note->tone != NOTE_END) {
        play_note(cur_note);
        cur_note++;
    }
}

note_t dot  = {NOTE_E5, LEN_SIXTEENTH};

note_t line = {NOTE_C5, LEN_DOTTED_SIXTEENTH};

//mapped by me :(
const note_t sik_hymn[] = { //C and F sharps
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

    {NOTE_D5, LEN_EIGHTH},
    {NOTE_E5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_E5, LEN_EIGHTH},
    {NOTE_E5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_CS5, LEN_DOTTED_QUARTER},
    {NOTE_CS5, LEN_EIGHTH},
    {NOTE_CS5, LEN_EIGHTH},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_CS5, LEN_EIGHTH},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_E5, LEN_QUARTER},
    {NOTE_FS5, LEN_QUARTER},
    {NOTE_REST, LEN_HALF},//3rd row

    {NOTE_A4, LEN_EIGHTH},
    {NOTE_A4, LEN_EIGHTH},
    {NOTE_FS5, LEN_QUARTER},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_E5, LEN_EIGHTH},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_CS5, LEN_QUARTER},
    {NOTE_B4, LEN_QUARTER},
    {NOTE_B4, LEN_EIGHTH},
    {NOTE_CS5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_E5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_CS5, LEN_QUARTER},
    {NOTE_A4, LEN_QUARTER},
    {NOTE_B4, LEN_QUARTER},//pie less sä

    {NOTE_B4, LEN_EIGHTH},//sillä arvonimi
    {NOTE_B4, LEN_EIGHTH},
    {NOTE_A4, LEN_EIGHTH},
    {NOTE_A4, LEN_EIGHTH},
    {NOTE_A4, LEN_EIGHTH},
    {NOTE_CS5, LEN_EIGHTH},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_E5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_G5, LEN_EIGHTH},
    {NOTE_A5, LEN_HALF},
    {NOTE_E5, LEN_QUARTER},
    {NOTE_A5, LEN_QUARTER},
    {NOTE_B5, LEN_QUARTER},
    {NOTE_B5, LEN_QUARTER},
    {NOTE_A5, LEN_QUARTER},
    {NOTE_G5, LEN_EIGHTH},
    {NOTE_A5, LEN_EIGHTH},
    {NOTE_FS5, LEN_HALF},
    {NOTE_FS5, LEN_QUARTER},
    {NOTE_G5, LEN_QUARTER},
    {NOTE_A5, LEN_HALF}, //kal
    {NOTE_AS5, LEN_HALF},//jaa
    {NOTE_B5, LEN_QUARTER},//juo
    {NOTE_FS5, LEN_QUARTER},
    {NOTE_E5, LEN_QUARTER},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_B4, LEN_HALF},
    {NOTE_A4, LEN_QUARTER},
    {NOTE_B4, LEN_QUARTER},

    {NOTE_B4, LEN_EIGHTH},//sillä arvonimi (chorus again)
    {NOTE_B4, LEN_EIGHTH},
    {NOTE_A4, LEN_EIGHTH},
    {NOTE_A4, LEN_EIGHTH},
    {NOTE_A4, LEN_EIGHTH},
    {NOTE_CS5, LEN_EIGHTH},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_D5, LEN_EIGHTH},
    {NOTE_E5, LEN_EIGHTH},
    {NOTE_FS5, LEN_EIGHTH},
    {NOTE_G5, LEN_EIGHTH},
    {NOTE_A5, LEN_HALF},
    {NOTE_E5, LEN_QUARTER},
    {NOTE_A5, LEN_QUARTER},
    {NOTE_B5, LEN_QUARTER},
    {NOTE_B5, LEN_QUARTER},
    {NOTE_A5, LEN_QUARTER},
    {NOTE_G5, LEN_EIGHTH},
    {NOTE_A5, LEN_EIGHTH},
    {NOTE_FS5, LEN_HALF},
    {NOTE_FS5, LEN_QUARTER},
    {NOTE_G5, LEN_QUARTER},
    {NOTE_A5, LEN_HALF}, //kal
    {NOTE_AS5, LEN_HALF},//jaa
    {NOTE_B5, LEN_QUARTER},//juo
    {NOTE_FS5, LEN_QUARTER},
    {NOTE_E5, LEN_QUARTER},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_D5, LEN_QUARTER},
    {NOTE_B4, LEN_HALF},
    {NOTE_A4, LEN_QUARTER},
    {NOTE_B4, LEN_QUARTER},

    {NOTE_B4, LEN_EIGHTH}, //cha cha cha
    {NOTE_B4, LEN_EIGHTH},
    {NOTE_B4, LEN_EIGHTH},

    {NOTE_REST, LEN_WHOLE},
    
    {NOTE_END, LEN_DOTTED_EIGHTH}//end

};