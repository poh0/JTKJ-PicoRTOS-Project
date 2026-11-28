// This library provides functions to initialize and use NON-BLOCKING buzzer functions using hardware PWM features that RP2040 has
// by gemini, prompt "go through the basics of implementing non blocking buzzer functions", some small edits made
// After first version, functions have been modified to use a more precise timer and be interrupt safe using critical sections

//comments added with prompt "add extensive comments"
/**
 * @file buzzer.c
 * @brief Non-blocking passive piezo buzzer driver for the TKJHAT (RP2040 + FreeRTOS).
 *
 * Architecture Overview:
 * ----------------------
 * Instead of toggling a GPIO pin in a software delay loop (which burns CPU cycles
 * and suffers pitch jitter whenever FreeRTOS preempts the calling task), this
 * driver separates tone generation from note timing across two hardware peripherals:
 *
 * 1. **Tone Generation (RP2040 Hardware PWM):**
 *    Once configured, the PWM slice generates a 0–50% duty-cycle square wave
 *    completely in hardware with zero CPU intervention. The PWM counter clock is
 *    divided down to 1 MHz so its 16-bit TOP (wrap) register covers the full
 *    audible range from 16 Hz up to ultrasonic frequencies.
 *
 * 2. **Note Timing (RP2040 64-bit Hardware Timer Alarms):**
 *    Note durations and inter-note articulation gaps are scheduled using the
 *    Pico SDK's microsecond hardware alarm pool (`pico/time.h`). Callbacks run
 *    directly inside the hardware timer IRQ—bypassing FreeRTOS task scheduling
 *    delays caused by OLED I2C transfers or USB polling—and return negative
 *    microsecond intervals to anchor transitions to exact timestamps without
 *    cumulative tempo drift.
 *
 * 3. **Task Synchronization (FreeRTOS Binary Semaphore):**
 *    All playback functions return immediately (non-blocking). Applications that
 *    feed sounds sequentially from a queue can optionally call `buzzer_wait_done()`
 *    in a dedicated audio task, putting that task into the FreeRTOS Blocked state
 *    (0% CPU usage) until the hardware alarm signals completion.
 */

#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "tkjhat/buzzer.h"

/**
 * @brief Silent gap (in milliseconds) inserted at the end of each audible melody note.
 *
 * Subtracted from each note's active sound duration so consecutive notes of the
 * same pitch (e.g., three quarter-note C5s) remain acoustically distinct without
 * stretching the overall tempo of the melody.
 */
#ifndef DELAY_BETWEEN_NOTES
#define DELAY_BETWEEN_NOTES 5U
#endif

/* -------------------------------------------------------------------------- */
/* Private Driver State                                                       */
/* -------------------------------------------------------------------------- */

/** Binary semaphore given when a tone, melody, or stop command completes. */
static SemaphoreHandle_t buzzer_done_sem = NULL;

/** RP2040 GPIO number and its mapped PWM slice/channel (GPIO 17 -> Slice 0, Ch B). */
static uint buzzer_pin;
static uint buzzer_slice;
static uint buzzer_channel;

/** Active Pico SDK hardware alarm ID (> 0 while a timer is armed, 0 when idle). */
static volatile alarm_id_t buzzer_alarm_id = 0;

/**
 * Melody playback state shared between application tasks and the hardware timer IRQ.
 * Marked `volatile` because `buzzer_alarm_cb()` mutates them inside an interrupt.
 */
static const buzzer_note_t * volatile active_melody = NULL;
static volatile size_t melody_len = 0;
static volatile size_t melody_idx = 0;
static volatile bool in_gap = false;

/** Output volume percentage (0..100%), mapped to a 0..50% PWM duty cycle. */
static uint8_t buzzer_volume = 100;

/** Currently sounding frequency in Hz (0 when muted/idle), cached for live volume updates. */
static uint32_t current_freq = 0;

/* -------------------------------------------------------------------------- */
/* Internal Synchronization & Interrupt Helpers                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Detect whether the CPU is currently executing inside an interrupt handler.
 *
 * Older RP2040 FreeRTOS ports do not expose `xPortIsInsideInterrupt()`. On the
 * ARM Cortex-M0+, reading the Interrupt Program Status Register (IPSR) returns 0
 * in Thread (task) mode and the active exception number (!= 0) in Handler (ISR) mode.
 *
 * @return true if executing inside a hardware IRQ/ISR, false if in normal task context.
 */
static inline bool buzzer_in_isr(void) {
    uint32_t ipsr;
    __asm volatile ("mrs %0, ipsr" : "=r" (ipsr));
    return (ipsr != 0U);
}

/**
 * @brief Drain any leftover token from `buzzer_done_sem` before starting new playback.
 *
 * Ensures a subsequent call to `buzzer_wait_done()` blocks until the newly
 * started tone or melody finishes rather than returning immediately on a stale
 * signal from an earlier `buzzer_stop()` or unawaited sound.
 */
static void buzzer_clear_done_sem(void) {
    if (buzzer_done_sem == NULL) {
        return;
    }

    if (buzzer_in_isr()) {
        BaseType_t woken = pdFALSE;
        xSemaphoreTakeFromISR(buzzer_done_sem, &woken);
        portYIELD_FROM_ISR(woken);
    } else {
        xSemaphoreTake(buzzer_done_sem, 0);
    }
}

/**
 * @brief Release `buzzer_done_sem` to wake any task waiting in `buzzer_wait_done()`.
 *
 * Automatically selects the ISR-safe FreeRTOS variant when called from the
 * hardware timer alarm callback or a GPIO button interrupt.
 */
static void buzzer_signal_done(void) {
    if (buzzer_done_sem == NULL) {
        return;
    }

    if (buzzer_in_isr()) {
        BaseType_t woken = pdFALSE;
        xSemaphoreGiveFromISR(buzzer_done_sem, &woken);
        portYIELD_FROM_ISR(woken);
    } else {
        xSemaphoreGive(buzzer_done_sem);
    }
}

/* -------------------------------------------------------------------------- */
/* Low-Level Hardware PWM Control                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Configure the RP2040 PWM slice for the requested pitch and current volume.
 *
 * Frequency & Wrap Calculation:
 *   The PWM counter is clocked at 1,000,000 Hz (1 tick = 1 us).
 *   To generate frequency `f`, one full PWM period requires `1,000,000 / f` ticks,
 *   so the 16-bit wrap (TOP) value is `(1,000,000 / f) - 1`.
 *   Frequencies below 16 Hz would require a wrap value > 65535 (overflowing the
 *   16-bit hardware counter), so any frequency < 16 Hz (including `NOTE_REST = 0`)
 *   disables the PWM output to produce clean silence.
 *
 * Volume & Duty Cycle Calculation:
 *   A passive piezo element achieves peak acoustic output at a 50% square-wave
 *   duty cycle (`level = wrap / 2`). Scaling `volume_percent` (0..100) via
 *   `(wrap * buzzer_volume) / 200` maps volume linearly across the 0%..50%
 *   duty-cycle range without altering the pitch.
 *
 * @param freq_hz Target tone frequency in Hz (16..20000 Hz), or 0 for silence.
 */
static void buzzer_hw_set_freq(uint32_t freq_hz) {
    current_freq = freq_hz;

    if (freq_hz < 16U || buzzer_volume == 0U) {
        pwm_set_chan_level(buzzer_slice, buzzer_channel, 0);
        pwm_set_enabled(buzzer_slice, false);
        return;
    }

    uint32_t wrap = (1000000UL / freq_hz) - 1U;
    uint16_t level = (uint16_t)((wrap * buzzer_volume) / 200U);

    pwm_set_wrap(buzzer_slice, (uint16_t)wrap);
    pwm_set_chan_level(buzzer_slice, buzzer_channel, level);
    pwm_set_enabled(buzzer_slice, true);
}

/* -------------------------------------------------------------------------- */
/* Hardware Timer Alarm State Machine (IRQ Context)                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief 64-bit hardware timer interrupt callback driving note and gap transitions.
 *
 * Pico SDK Return Value Semantics:
 *   - Returning `0` cancels/completes the alarm without rescheduling.
 *   - Returning a **negative** value `-us` reschedules the next alarm `us`
 *     microseconds after the *exact target timestamp* of this interrupt (rather
 *     than `now()`), eliminating cumulative timing drift across long melodies.
 *
 * @param id        Hardware alarm identifier that triggered this callback.
 * @param user_data Unused context pointer.
 * @return Next relative interval in negative microseconds, or 0 when finished.
 */
static int64_t buzzer_alarm_cb(alarm_id_t id, void *user_data) {
    (void)user_data;

    /* Guard against a race where buzzer_stop() canceled this alarm right as its IRQ fired. */
    if (buzzer_alarm_id != id) {
        return 0;
    }

    /* Case 1: Single asynchronous tone (`buzzer_tone_async`) has finished. */
    if (active_melody == NULL) {
        buzzer_alarm_id = 0;
        buzzer_hw_set_freq(0);
        buzzer_signal_done();
        return 0;
    }

    /* Case 2: Audible phase of the current melody note ended -> enter articulation gap. */
    if (!in_gap) {
        in_gap = true;
        buzzer_hw_set_freq(NOTE_REST);
        return -((int64_t)DELAY_BETWEEN_NOTES * 1000LL);
    }

    /* Case 3: Trailing gap ended -> advance to the next note in the melody array. */
    melody_idx++;
    if (melody_idx < melody_len && active_melody[melody_idx].freq_hz != NOTE_END) {
        buzzer_note_t note = active_melody[melody_idx];

        /* Split audible notes longer than DELAY_BETWEEN_NOTES into (sound + gap). */
        if (note.freq_hz != NOTE_REST && note.duration_ms > DELAY_BETWEEN_NOTES) {
            in_gap = false;
            buzzer_hw_set_freq(note.freq_hz);
            return -((int64_t)(note.duration_ms - DELAY_BETWEEN_NOTES) * 1000LL);
        }

        /* Rests and ultra-short notes (<= DELAY_BETWEEN_NOTES) run unsplit for full duration. */
        in_gap = true;
        buzzer_hw_set_freq(note.freq_hz);
        return -((int64_t)note.duration_ms * 1000LL);
    }

    /* Case 4: Reached end of array or `NOTE_END` sentinel -> clean up and notify waiters. */
    active_melody = NULL;
    melody_len = 0;
    melody_idx = 0;
    in_gap = false;
    buzzer_alarm_id = 0;
    buzzer_hw_set_freq(0);
    buzzer_signal_done();
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */

void buzzer_init(int pin) {
    buzzer_pin = (uint)pin;

    /* Route the GPIO pin to the RP2040 hardware PWM peripheral. */
    gpio_set_function(buzzer_pin, GPIO_FUNC_PWM);
    buzzer_slice = pwm_gpio_to_slice_num(buzzer_pin);
    buzzer_channel = pwm_gpio_to_channel(buzzer_pin);

    /* Configure the PWM slice clock divider to produce an exact 1 MHz tick rate
     * regardless of whether clk_sys is running at 125 MHz or 133 MHz. */
    pwm_config cfg = pwm_get_default_config();
    float div = (float)clock_get_hz(clk_sys) / 1000000.0f;
    pwm_config_set_clkdiv(&cfg, div);

    /* Initialize slice in disabled state so the buzzer remains silent until triggered. */
    pwm_init(buzzer_slice, &cfg, false);

    /* Create the completion semaphore once (safe if buzzer_init is called repeatedly). */
    if (buzzer_done_sem == NULL) {
        buzzer_done_sem = xSemaphoreCreateBinary();
    }
}

void buzzer_stop(void) {
    /* Enter a critical section so the timer IRQ cannot modify state mid-cancellation. */
    uint32_t irq_state = save_and_disable_interrupts();
    if (buzzer_alarm_id > 0) {
        cancel_alarm(buzzer_alarm_id);
        buzzer_alarm_id = 0;
    }
    active_melody = NULL;
    melody_len = 0;
    melody_idx = 0;
    in_gap = false;
    buzzer_hw_set_freq(0);
    restore_interrupts(irq_state);

    /* Unblock any task waiting in buzzer_wait_done() so canceling never deadlocks. */
    buzzer_signal_done();
}

void buzzer_tone_async(uint32_t freq_hz, uint32_t duration_ms) {
    if (duration_ms == 0U) {
        return;
    }

    buzzer_clear_done_sem();

    /* Atomically cancel any active tone/melody and schedule the new one-shot alarm. */
    uint32_t irq_state = save_and_disable_interrupts();
    if (buzzer_alarm_id > 0) {
        cancel_alarm(buzzer_alarm_id);
        buzzer_alarm_id = 0;
    }
    active_melody = NULL;
    in_gap = false;
    buzzer_hw_set_freq(freq_hz);
    buzzer_alarm_id = add_alarm_in_us((uint64_t)duration_ms * 1000ULL, buzzer_alarm_cb, NULL, false);
    restore_interrupts(irq_state);
}

void buzzer_play_melody_async(const buzzer_note_t *notes, size_t count) {
    if (notes == NULL || count == 0U || notes[0].freq_hz == NOTE_END) {
        return;
    }

    buzzer_clear_done_sem();

    /* Atomically replace any ongoing playback with the new melody sequence. */
    uint32_t irq_state = save_and_disable_interrupts();
    if (buzzer_alarm_id > 0) {
        cancel_alarm(buzzer_alarm_id);
        buzzer_alarm_id = 0;
    }

    active_melody = notes;
    melody_len = count;
    melody_idx = 0;

    /* Start note 0 immediately and calculate the first alarm interval (sound phase vs. full). */
    buzzer_note_t first = notes[0];
    uint32_t first_delay_ms;
    if (first.freq_hz != NOTE_REST && first.duration_ms > DELAY_BETWEEN_NOTES) {
        in_gap = false;
        buzzer_hw_set_freq(first.freq_hz);
        first_delay_ms = first.duration_ms - DELAY_BETWEEN_NOTES;
    } else {
        in_gap = true;
        buzzer_hw_set_freq(first.freq_hz);
        first_delay_ms = first.duration_ms;
    }

    buzzer_alarm_id = add_alarm_in_us((uint64_t)first_delay_ms * 1000ULL, buzzer_alarm_cb, NULL, false);
    restore_interrupts(irq_state);
}

void buzzer_set_volume(uint8_t volume_percent) {
    if (volume_percent > 100U) {
        volume_percent = 100U;
    }
    buzzer_volume = volume_percent;

    /* Apply the updated duty cycle immediately if a tone is currently active. */
    uint32_t irq_state = save_and_disable_interrupts();
    if (current_freq >= 16U) {
        buzzer_hw_set_freq(current_freq);
    }
    restore_interrupts(irq_state);
}

void buzzer_wait_done(void) {
    if (buzzer_done_sem != NULL) {
        xSemaphoreTake(buzzer_done_sem, portMAX_DELAY);
    }
}