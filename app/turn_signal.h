#ifndef TURN_SIGNAL_H
#define TURN_SIGNAL_H

/*
 * Turn signal state machine - pure C, no hardware.
 *
 * Builds and runs on the PC (unit tests) and on the TM4C (firmware) unchanged.
 * Do NOT include any register or driver header here (gpio.h, uart.h, ...).
 *
 * Transition table (design.md). Pressing the button of the side that is
 * already on turns it off (toggle), because momentary buttons have no
 * "centre" position like a real stalk.
 *
 *     state \ event  |  LEFT   |  RIGHT
 *     ---------------+---------+--------
 *     OFF            |  LEFT   |  RIGHT
 *     LEFT           |  OFF    |  RIGHT
 *     RIGHT          |  LEFT   |  OFF
 */

typedef enum {
    STATE_OFF   = 0,
    STATE_LEFT  = 1,
    STATE_RIGHT = 2,
    STATE_COUNT          /* keep last: number of states, sizes the table */
} state_t;

typedef enum {
    EVENT_LEFT  = 0,     /* LEFT button pressed (one debounced press)  */
    EVENT_RIGHT = 1,     /* RIGHT button pressed                       */
    EVENT_COUNT          /* keep last: number of events, sizes the table */
} event_t;

/*
 * Return the state after `event` happens in `state`.
 * Out-of-range state or event: returns `state` unchanged (never reads
 * outside the table).
 */
state_t state_machine_step(state_t state, event_t event);

#endif /* TURN_SIGNAL_H */
