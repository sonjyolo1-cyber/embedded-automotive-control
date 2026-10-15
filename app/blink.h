#ifndef BLINK_H
#define BLINK_H

/*
 * Blink logic for the turn signal lamps - pure C, no hardware.
 *
 * Input : the turn signal state + the current time in ms (passed in by main,
 *         so tests can feed "fake ticks" like 0, 333, 666 ...).
 * Output: on/off for the left side and the right side. main only writes
 *         these to the lamps; every decision is made (and tested) here.
 *
 *   state   | left      | right
 *   --------+-----------+----------
 *   OFF     | 0         | 0
 *   LEFT    | phase_on  | 0
 *   RIGHT   | 0         | phase_on
 *   HAZARD  | phase_on  | phase_on
 *
 * Rules (design.md):
 *  - When the state changes, the phase restarts ON at that moment,
 *    so the lamp lights immediately after a press.
 *  - Every BLINK_HALF_PERIOD_MS the phase flips, with last_ms += period
 *    (fixed time grid, no drift, wrap-around safe).
 */

#include <stdint.h>
#include "turn_signal.h"   /* state_t */

#define BLINK_HALF_PERIOD_MS  333U   /* 1.5 Hz: 333 ms on, 333 ms off */

typedef struct {
    state_t  prev_state;  /* state seen on the previous call, to detect a change */
    uint32_t last_ms;     /* start time of the current phase                     */
    uint8_t  phase_on;    /* current phase: 1 = on, 0 = off                       */
} blink_t;

typedef struct {
    uint8_t left;         /* 1 = left lamps (PE0, PE1) on   */
    uint8_t right;        /* 1 = right lamps (PE2, PE3) on  */
} blink_out_t;

/* Start as if the state were OFF, so the first press counts as a change. */
void blink_init(blink_t *b, uint32_t now_ms);

/*
 * Call once per main loop pass. Returns which sides must be lit now.
 * A corrupted state (>= STATE_COUNT) gives all lamps off.
 */
blink_out_t blink_update(blink_t *b, state_t state, uint32_t now_ms);

#endif /* BLINK_H */
