#include "turn_signal.h"

/*
 * The transition table from design.md, copied cell for cell.
 * Row = current state, column = event, value = next state.
 * Its size comes from the *_COUNT enum members, so the compiler always
 * reserves exactly one cell per (state, event) pair.
 */
static const state_t next_state[STATE_COUNT][EVENT_COUNT] = {
    /*                 EVENT_LEFT    EVENT_RIGHT */
    /* STATE_OFF   */ { STATE_LEFT,  STATE_RIGHT },
    /* STATE_LEFT  */ { STATE_OFF,   STATE_RIGHT },
    /* STATE_RIGHT */ { STATE_LEFT,  STATE_OFF   },
};

state_t state_machine_step(state_t state, event_t event)
{
    /* C does not check array bounds: an out-of-range index would silently
     * read another cell or memory outside the table. Refuse it instead.
     * The (unsigned) casts also reject negative values. */
    if (((unsigned)state >= (unsigned)STATE_COUNT) ||
        ((unsigned)event >= (unsigned)EVENT_COUNT)) {
        return state;
    }

    return next_state[state][event];
}
