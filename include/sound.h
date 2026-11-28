#pragma once

#define NOTE_DELAY 10

//sheet notation of frequencies and lenghts mapped into 2 enums: note_length_t and note_freq_t
//by gemini, prompt "convert into 2 enums" after giving 2 lookuptables and enums doing the same thing...
typedef enum {
    LEN_WHOLE            = 2000, // Whole note (1)
    LEN_DOTTED_HALF      = 1500, // Dotted half note (1.5x half)
    LEN_HALF             = 1000, // Half note (1/2)
    LEN_DOTTED_QUARTER   = 750,  // Dotted quarter note
    LEN_QUARTER          = 500,  // Quarter note (1/4) - 1 beat at 120 BPM
    LEN_DOTTED_EIGHTH    = 375,  // Dotted eighth note
    LEN_EIGHTH           = 250,  // Eighth note (1/8)
    LEN_DOTTED_SIXTEENTH = 187,  // Dotted sixteenth note (rounded from 187.5)
    LEN_SIXTEENTH        = 125   // Sixteenth note (1/16)
} note_length_t;

typedef enum {
    NOTE_REST = 0,     // 0 Hz (Silence)

    // Octave 4
    NOTE_C4   = 262,
    NOTE_CS4  = 277,   // C# / Db
    NOTE_D4   = 294,
    NOTE_DS4  = 311,   // D# / Eb
    NOTE_E4   = 330,
    NOTE_F4   = 349,
    NOTE_FS4  = 370,   // F# / Gb
    NOTE_G4   = 392,
    NOTE_GS4  = 415,   // G# / Ab
    NOTE_A4   = 440,
    NOTE_AS4  = 466,   // A# / Bb
    NOTE_B4   = 494,

    // Octave 5
    NOTE_C5   = 523,
    NOTE_CS5  = 554,
    NOTE_D5   = 587,
    NOTE_DS5  = 622,
    NOTE_E5   = 659,
    NOTE_F5   = 698,
    NOTE_FS5  = 740,
    NOTE_G5   = 784,
    NOTE_GS5  = 831,
    NOTE_A5   = 880,
    NOTE_AS5  = 932,
    NOTE_B5   = 988,

    // Octave 6
    NOTE_C6   = 1047,
    NOTE_CS6  = 1109,
    NOTE_D6   = 1175,
    NOTE_DS6  = 1245,
    NOTE_E6   = 1319,
    NOTE_F6   = 1397,
    NOTE_FS6  = 1480,
    NOTE_G6   = 1568,
    NOTE_GS6  = 1661,
    NOTE_A6   = 1760,
    NOTE_AS6  = 1865,
    NOTE_B6   = 1976,

    NOTE_END  = 0xFFFF // Sentinel value to mark end of a song
} note_freq_t;

typedef struct{
    note_freq_t tone;
    note_length_t length;
} note_t;

typedef enum {
    SOUND_DOT = 0,
    SOUND_DASH,
    SOUND_SONG_SIK_HYMN
} sound_event_t;

extern const note_t sik_hymn[];
extern const note_t dot;
extern const note_t line;

void play_note(note_t * n);
void play_song(note_t * song);
