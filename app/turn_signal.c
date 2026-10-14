#include <stddef.h>
#include "turn_signal.h"

/*
 * The stalk table only covers OFF/LEFT/RIGHT and the LEFT/RIGHT events.
 * HAZARD is handled by the if-logic below, because it reads and writes
 * resume_state. These sizes rely on the enum order in turn_signal.h:
 * STATE_HAZARD comes right after the stalk states, EVENT_HAZARD right
 * after the stalk events.
 */
#define STALK_STATE_COUNT  ((unsigned)STATE_HAZARD)   /* 3: OFF, LEFT, RIGHT */
#define STALK_EVENT_COUNT  ((unsigned)EVENT_HAZARD)   /* 2: LEFT, RIGHT      */

static const state_t stalk_table[STALK_STATE_COUNT][STALK_EVENT_COUNT] = {
    /*                 EVENT_LEFT    EVENT_RIGHT */
    /* STATE_OFF   */ { STATE_LEFT,  STATE_RIGHT },
    /* STATE_LEFT  */ { STATE_OFF,   STATE_RIGHT },
    /* STATE_RIGHT */ { STATE_LEFT,  STATE_OFF   },
};

/* One move of the stalk. Anything outside the table: no move. */
static state_t stalk_toggle(state_t stalk, event_t event)
{
    if (((unsigned)stalk >= STALK_STATE_COUNT) ||
        ((unsigned)event >= STALK_EVENT_COUNT)) {
        return stalk;
    }
    return stalk_table[stalk][event];
}

void turn_signal_init(turn_signal_t *ts)
{
    if (ts == NULL) {
        return;
    }
    ts->state        = STATE_OFF;
    ts->resume_state = STATE_OFF;
}

void turn_signal_step(turn_signal_t *ts, event_t event)
{
    if (ts == NULL) {
        return;
    }
    /* Guard: C does not check array bounds, so refuse bad inputs here. */
    if (((unsigned)event     >= (unsigned)EVENT_COUNT) ||
        ((unsigned)ts->state >= (unsigned)STATE_COUNT)) {
        return;
    }

    if (ts->state == STATE_HAZARD) {
        if (event == EVENT_HAZARD) {
            /* Hazard off: hand control back to the virtual stalk. */
            ts->state = ts->resume_state;
        } else {
            /* Lamps stay on hazard; only the virtual stalk moves. */
            ts->resume_state = stalk_toggle(ts->resume_state, event);
        }
    } else {
        if (event == EVENT_HAZARD) {
            /* Hazard on: remember where the stalk is, then override. */
            ts->resume_state = ts->state;
            ts->state        = STATE_HAZARD;
        } else {
            ts->state = stalk_toggle(ts->state, event);
        }
    }
}