#include "music.h"

//lookup tables for note lengths and frequencies
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

