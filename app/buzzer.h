#ifndef BUZZER_H
#define BUZZER_H

/*
 * Buzzer tick logic (R11) - pure C, no hardware.
 *
 * Input : lamp_on = 1 when any turn lamp is lit right now (out.left || out.right
 *         from blink_update), plus the time in ms (passed in by main).
 * Output: the level for the buzzer pin (1 = transistor on).
 *
 * A passive buzzer only clicks when the coil current CHANGES, so the pin gives a
 * short pulse at every lamp edge (on and off) and stays 0 the rest of the time:
 *
 *   lamp_on : ____|‾‾‾‾‾‾‾‾‾‾‾|___________|‾‾‾‾‾‾‾‾
 *   pin     : ____|‾|_________|‾|_________|‾|______     ("tich", "tac", "tich")
 *                  <-> BUZZER_PULSE_MS
 *
 * Holding the pin at 1 for the whole 333 ms phase would pass ~200 mA through the
 * coil (3.3 V / 16 ohm) without making the click any louder.
 *
 * Following the lamp output (not blink's internal phase) keeps the buzzer silent in
 * OFF and clicks at once when a signal starts. Edges that arrive together (blink
 * catching up after a stalled main) only restart the pulse, so they give one click.
 */

#include <stdint.h>

/* Pulse length in ms. Tick resolution is 1 ms, so the real pulse is between
 * BUZZER_PULSE_MS - 1 and BUZZER_PULSE_MS ms: keep it >= 2 (1 could be ~0 ms).
 * Tune by ear on the board. */
#define BUZZER_PULSE_MS  2U

typedef struct {
    uint8_t  prev_lamp;       /* lamp_on seen on the previous call, to detect an edge */
    uint32_t pulse_start_ms;  /* time of the latest edge                               */
    uint8_t  active;          /* 1 = pulse in progress                                 */
} buzzer_t;

/* Lamps off, no pulse. */
void buzzer_init(buzzer_t *bz, uint32_t now_ms);

/* Call once per main loop pass, after blink_update. Returns the buzzer pin level. */
uint8_t buzzer_update(buzzer_t *bz, uint8_t lamp_on, uint32_t now_ms);

#endif /* BUZZER_H */
