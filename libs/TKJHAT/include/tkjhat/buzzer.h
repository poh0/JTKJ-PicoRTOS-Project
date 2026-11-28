//header files by gemini AI, prompt "create a corresponding header file (buzzer.h)", small edits made
//after updating the c file, header was also updated to match c file content.

/**
 * @file buzzer.h
 * @brief Non-blocking passive piezo buzzer driver API for the TKJHAT (RP2040 + FreeRTOS).
 *
 * Uses RP2040 hardware PWM for tone generation and 64-bit hardware timer alarms
 * (`pico/time.h`) for microsecond-accurate, jitter-free note timing and inter-note
 * articulation gaps. All playback functions return immediately without blocking
 * the calling FreeRTOS task.
 */

#ifndef TKJHAT_BUZZER_H
#define TKJHAT_BUZZER_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Default silent articulation gap (in milliseconds) inserted at the end of
 *        each audible melody note so consecutive notes of equal pitch remain distinct.
 */
#ifndef DELAY_BETWEEN_NOTES
#define DELAY_BETWEEN_NOTES 10U
#endif

/**
 * @brief Helper macro to compute the number of elements in a static melody array.
 *
 * @warning Only use on actual array identifiers in the same scope—never on pointer
 *          parameters where `sizeof` decays to pointer size.
 */
#define BUZZER_MELODY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief Standard musical note durations in milliseconds (based on 120 BPM).
 */
typedef enum {
    LEN_WHOLE            = 2000, /**< Whole note (4 beats) */
    LEN_DOTTED_HALF      = 1500, /**< Dotted half note (3 beats) */
    LEN_HALF             = 1000, /**< Half note (2 beats) */
    LEN_DOTTED_QUARTER   = 750,  /**< Dotted quarter note (1.5 beats) */
    LEN_QUARTER          = 500,  /**< Quarter note (1 beat at 120 BPM) */
    LEN_DOTTED_EIGHTH    = 375,  /**< Dotted eighth note (0.75 beats) */
    LEN_EIGHTH           = 250,  /**< Eighth note (0.5 beats) */
    LEN_DOTTED_SIXTEENTH = 187,  /**< Dotted sixteenth note (0.375 beats, rounded) */
    LEN_SIXTEENTH        = 125   /**< Sixteenth note (0.25 beats) */
} note_length_t;

/**
 * @brief Standard musical note frequencies in Hz (Octaves 4–6) and control tokens.
 */
typedef enum {
    NOTE_REST = 0,     /**< Silent rest (0 Hz) */

    /* Octave 4 */
    NOTE_C4   = 262,   /**< C4 (Middle C) */
    NOTE_CS4  = 277,   /**< C#4 / Db4 */
    NOTE_D4   = 294,   /**< D4 */
    NOTE_DS4  = 311,   /**< D#4 / Eb4 */
    NOTE_E4   = 330,   /**< E4 */
    NOTE_F4   = 349,   /**< F4 */
    NOTE_FS4  = 370,   /**< F#4 / Gb4 */
    NOTE_G4   = 392,   /**< G4 */
    NOTE_GS4  = 415,   /**< G#4 / Ab4 */
    NOTE_A4   = 440,   /**< A4 (Concert Pitch) */
    NOTE_AS4  = 466,   /**< A#4 / Bb4 */
    NOTE_B4   = 494,   /**< B4 */

    /* Octave 5 */
    NOTE_C5   = 523,   /**< C5 */
    NOTE_CS5  = 554,   /**< C#5 / Db5 */
    NOTE_D5   = 587,   /**< D5 */
    NOTE_DS5  = 622,   /**< D#5 / Eb5 */
    NOTE_E5   = 659,   /**< E5 */
    NOTE_F5   = 698,   /**< F5 */
    NOTE_FS5  = 740,   /**< F#5 / Gb5 */
    NOTE_G5   = 784,   /**< G5 */
    NOTE_GS5  = 831,   /**< G#5 / Ab5 */
    NOTE_A5   = 880,   /**< A5 */
    NOTE_AS5  = 932,   /**< A#5 / Bb5 */
    NOTE_B5   = 988,   /**< B5 */

    /* Octave 6 */
    NOTE_C6   = 1047,  /**< C6 */
    NOTE_CS6  = 1109,  /**< C#6 / Db6 */
    NOTE_D6   = 1175,  /**< D6 */
    NOTE_DS6  = 1245,  /**< D#6 / Eb6 */
    NOTE_E6   = 1319,  /**< E6 */
    NOTE_F6   = 1397,  /**< F6 */
    NOTE_FS6  = 1480,  /**< F#6 / Gb6 */
    NOTE_G6   = 1568,  /**< G6 */
    NOTE_GS6  = 1661,  /**< G#6 / Ab6 */
    NOTE_A6   = 1760,  /**< A6 */
    NOTE_AS6  = 1865,  /**< A#6 / Bb6 */
    NOTE_B6   = 1976,  /**< B6 */

    NOTE_END  = 0xFFFF /**< Optional sentinel value to terminate a melody array */
} note_freq_t;

/**
 * @brief Single note or rest entry for asynchronous melody playback.
 *
 * Accepts values from @ref note_freq_t and @ref note_length_t or raw Hz/ms integers.
 */
typedef struct {
    uint16_t freq_hz;      /**< Tone frequency in Hz (use NOTE_REST / 0 for silence). */
    uint16_t duration_ms;  /**< Total duration of the note or rest in milliseconds. */
} buzzer_note_t;

/**
 * @brief Initialize the buzzer GPIO for hardware PWM output and create its sync semaphore.
 *
 * Configures the PWM slice clock divider to 1 MHz so the 16-bit counter covers
 * frequencies from 16 Hz upward. Call once after `init_hat_sdk()` and before
 * triggering playback. On the TKJHAT board, the piezo buzzer is wired to GPIO 17.
 *
 * @param pin RP2040 GPIO pin connected to the buzzer (e.g., `BUZZER_PIN` / `17`).
 */
void buzzer_init(int pin);

/**
 * @brief Immediately silence the buzzer, cancel active hardware alarms, and unblock waiters.
 *
 * Safe to call from both FreeRTOS tasks and hardware interrupt handlers (ISRs).
 */
void buzzer_stop(void);

/**
 * @brief Play a single tone asynchronously without blocking the calling context.
 *
 * Cancels any active tone or melody before starting. Frequencies below 16 Hz
 * produce silence for the specified duration. Safe to call from tasks or ISRs.
 *
 * @param freq_hz     Tone frequency in Hz (16 Hz to ~20 kHz, or `NOTE_REST` / 0 for silence).
 * @param duration_ms Duration in milliseconds before automatically turning off.
 */
void buzzer_tone_async(uint32_t freq_hz, uint32_t duration_ms);

/**
 * @brief Play a sequence of notes asynchronously in the background.
 *
 * Each audible note longer than @ref DELAY_BETWEEN_NOTES is automatically split into
 * an active tone phase followed by a trailing silent gap while preserving exact total
 * note timing. Playback stops when @p count elements are played or @ref NOTE_END is reached.
 * Safe to call from tasks or ISRs.
 *
 * @warning The @p notes pointer is read asynchronously from hardware timer interrupts across
 *          note transitions. Pass only persistent (`static` or `const`) arrays—never
 *          stack-allocated local arrays that go out of scope before playback finishes.
 *
 * @param notes Pointer to a persistent array of @ref buzzer_note_t elements.
 * @param count Number of elements in the @p notes array (use @ref BUZZER_MELODY_LEN).
 */
void buzzer_play_melody_async(const buzzer_note_t *notes, size_t count);

/**
 * @brief Set the buzzer output volume.
 *
 * Maps 0–100% volume to a 0–50% PWM duty cycle. Takes effect immediately if a tone
 * is currently sounding.
 *
 * @param volume_percent Volume level from `0` (mute) to `100` (maximum loudness).
 */
void buzzer_set_volume(uint8_t volume_percent);

/**
 * @brief Block the calling FreeRTOS task until the active tone or melody completes.
 *
 * Puts the calling task into the FreeRTOS Blocked state (0% CPU usage) until the
 * hardware timer finishes playback or `buzzer_stop()` is called. Useful inside a
 * dedicated audio consumer task draining a sound command queue.
 *
 * @warning Must only be called from a FreeRTOS task context—never from an ISR.
 */
void buzzer_wait_done(void);

#endif /* TKJHAT_BUZZER_H */