#include <stddef.h>
#include "blink.h"

void blink_init(blink_t *b, uint32_t now_ms)
{
    if (b == NULL) {
        return;
    }
    b->prev_state = STATE_OFF;
    b->last_ms    = now_ms;
    b->phase_on   = 0U;
}

blink_out_t blink_update(blink_t *b, state_t state, uint32_t now_ms)
{
    blink_out_t out = { 0U, 0U };   /* safe default: all lamps off */

    if (b == NULL) {
        return out;
    }
    /* Guard: unknown state -> lamps off, memory untouched. */
    if ((unsigned)state >= (unsigned)STATE_COUNT) {
        return out;
    }

    /* 1. State changed? Restart the phase ON, counted from now. */
    if (state != b->prev_state) {
        b->prev_state = state;
        b->last_ms    = now_ms;
        b->phase_on   = 1U;
    }

    /* 2. Half period over? Flip the phase, move the mark along the grid.
     *    Unsigned subtraction stays correct across the 32-bit wrap.
     *    At most one flip per call: after a long stall, a few quick calls
     *    catch up with the grid (see design.md, risks). */
    if ((uint32_t)(now_ms - b->last_ms) >= BLINK_HALF_PERIOD_MS) {
        b->phase_on = (uint8_t)!b->phase_on;
        b->last_ms += BLINK_HALF_PERIOD_MS;
    }

    /* 3. Which sides follow the phase. */
    switch (state) {
    case STATE_LEFT:
        out.left  = b->phase_on;
        break;
    case STATE_RIGHT:
        out.right = b->phase_on;
        break;
    case STATE_HAZARD:
        out.left  = b->phase_on;
        out.right = b->phase_on;
        break;
    case STATE_OFF:
    default:
        break;                      /* stays { 0, 0 } */
    }
    return out;
}
