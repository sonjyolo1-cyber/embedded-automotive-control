#ifndef PRESS_FILTER_H
#define PRESS_FILTER_H

/*
 * LEFT + RIGHT conflict filter - pure C, no hardware.
 *
 * Input : one loop pass of button flags. main fills them from
 *         button_was_pressed (edge, this pass) and button_is_pressed (held now).
 * Output: the events to pass to turn_signal_step, always in the order
 *         LEFT, RIGHT, HAZARD, so a run on the board equals a run in the test.
 *
 * Rules (design.md, "LEFT + RIGHT conflict"):
 *  1. A LEFT press is dropped while RIGHT is held, and the other way round:
 *     the button that came first has already acted, the later one loses.
 *  2. Both edges in the same pass: both dropped, neither came first. Checked on
 *     its own, not left to rule 1, so it holds however the driver sets "held".
 *  3. HAZARD is never in conflict and always passes.
 *
 * out must hold EVENT_COUNT entries. The size in the prototype is only a hint
 * (an array parameter is a pointer in C); each event is written at most once,
 * so at most EVENT_COUNT entries are written.
 */

#include <stdint.h>
#include "turn_signal.h"   /* event_t, EVENT_COUNT */

typedef struct {
    uint8_t left_pressed;    /* button_was_pressed(BUTTON_LEFT)   this pass */
    uint8_t right_pressed;   /* button_was_pressed(BUTTON_RIGHT)  this pass */
    uint8_t hazard_pressed;  /* button_was_pressed(BUTTON_HAZARD) this pass */
    uint8_t left_held;       /* button_is_pressed(BUTTON_LEFT)  now         */
    uint8_t right_held;      /* button_is_pressed(BUTTON_RIGHT) now         */
} press_filter_in_t;

/*
 * Writes the events to send into out and returns how many (0..EVENT_COUNT).
 * in or out NULL: writes nothing, returns 0.
 */
uint8_t press_filter_events(const press_filter_in_t *in, event_t out[EVENT_COUNT]);

#endif /* PRESS_FILTER_H */
