#ifndef TURN_SIGNAL_H
#define TURN_SIGNAL_H

/*
 * Turn signal + hazard state machine - pure C, no hardware.
 *
 * Builds and runs on the PC (unit tests) and on the TM4C (firmware) unchanged.
 * Do NOT include any register or driver header here (gpio.h, uart.h, ...).
 *
 * Two rules (design.md):
 *
 * 1. LEFT/RIGHT follow the "stalk" toggle table. Pressing the side that is
 *    already on turns it off, because momentary buttons have no centre
 *    position like a real stalk.
 *
 *        stalk \ press |  LEFT  |  RIGHT
 *        --------------+--------+-------
 *        OFF           |  LEFT  |  RIGHT
 *        LEFT          |  OFF   |  RIGHT
 *        RIGHT         |  LEFT  |  OFF
 *
 * 2. HAZARD has the highest priority. While it is on, LEFT/RIGHT do not
 *    change the lamps, but they still move a "virtual stalk" (resume_state)
 *    through the same table. Pressing HAZARD again returns to resume_state,
 *    like a real car whose latched stalk takes over when hazard goes off.
 */

typedef enum {
    STATE_OFF    = 0,
    STATE_LEFT   = 1,
    STATE_RIGHT  = 2,
    STATE_HAZARD = 3,
    STATE_COUNT          /* keep last: number of states */
} state_t;

typedef enum {
    EVENT_LEFT   = 0,    /* LEFT button pressed (one debounced press) */
    EVENT_RIGHT  = 1,    /* RIGHT button pressed                      */
    EVENT_HAZARD = 2,    /* HAZARD button pressed                     */
    EVENT_COUNT          /* keep last: number of events */
} event_t;

typedef struct {
    state_t state;         /* what the lamps show: OFF, LEFT, RIGHT, HAZARD     */
    state_t resume_state;  /* virtual stalk: OFF, LEFT or RIGHT, never HAZARD;  */
                           /* the state to return to when HAZARD goes off       */
} turn_signal_t;

/* Start in OFF with the virtual stalk in the centre (OFF). */
void turn_signal_init(turn_signal_t *ts);

/*
 * Apply one button event. An out-of-range event or a corrupted state
 * leaves *ts unchanged (never reads outside the table).
 */
void turn_signal_step(turn_signal_t *ts, event_t event);

#endif /* TURN_SIGNAL_H */