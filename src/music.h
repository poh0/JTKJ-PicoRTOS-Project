//usage: 
//init_buzzer();
//play_song(song);

#pragma once
#ifndef MUSIC_H
#define MUSIC_H
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

typedef struct{
    buzzer_note tone;
    note_length length;
}note;

extern const unsigned int note_durations_ms[NUM_NOTE_LENGTHS];
extern const unsigned int buzzer_frequencies[NUM_BUZZER_NOTES];

void play_note(note * n);
void play_song(note * song);

#endif