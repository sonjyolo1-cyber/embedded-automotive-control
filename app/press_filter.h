#ifndef PRESS_FILTER_H
#define PRESS_FILTER_H

#include <stdint.h>
#include "turn_signal.h"

typedef struct {
    uint8_t left_pressed, right_pressed, hazard_pressed;  /* button_was_pressed */
    uint8_t left_held, right_held;                          /* button_is_pressed */
} press_filter_in_t;

uint8_t press_filter_events(const press_filter_in_t *in, event_t out[EVENT_COUNT]);

#endif