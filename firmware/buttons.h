#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

#define BUTTON_COUNT 3U

typedef enum {
	BUTTON_LEFT = 0,
	BUTTON_RIGHT = 1,
	BUTTON_HAZARD = 2
}buttons_id;

void buttons_init(void);

uint8_t read_raw(buttons_id button);

void buttons_update(uint32_t now_ms);

uint8_t button_is_pressed(buttons_id button);

uint8_t button_was_pressed(buttons_id button);
#endif