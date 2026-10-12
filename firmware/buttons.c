#include "gpio.h"
#include "buttons.h"

#define BUTTON_GPIO_PORT GPIO_PORTB_BASE
#define BUTTON_PIN_MASK 0x07U

#define BUTTON_ACTIVE_LEVEL 0U
#define BOUNCE_MS 20U

typedef struct {
	uint8_t raw_prev;
	uint32_t last_edge_ms;
	uint8_t stable;
	uint8_t pressed_state;
}button_state;

static button_state s_button[BUTTON_COUNT];

uint8_t read_raw(buttons_id button) {
	uint8_t level = gpio_read_pin(BUTTON_GPIO_PORT, (uint8_t)button);
	return (level == BUTTON_ACTIVE_LEVEL) ? 1U : 0U;
}

void buttons_init(void) {
	gpio_enable_clock(SYSCTL_RCGCGPIO_PORTB);
	
	gpio_set_input(BUTTON_GPIO_PORT, BUTTON_PIN_MASK);
	
	gpio_enable_pullup(BUTTON_GPIO_PORT, BUTTON_PIN_MASK);
	
	gpio_enable_digital(BUTTON_GPIO_PORT, BUTTON_PIN_MASK);
	
	for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
		uint8_t raw = read_raw((buttons_id)i);
		s_button[i].raw_prev = raw;
		s_button[i].stable = raw;
		s_button[i].last_edge_ms = 0U;
		s_button[i].pressed_state = 0U;
	}
}

void buttons_update(uint32_t now_ms) {
	for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
		button_state *b = &s_button[i];
		uint8_t raw = read_raw((buttons_id)i);
		
		if (raw != b->raw_prev) {
			b->raw_prev = raw;
			b->last_edge_ms = now_ms;
		}
		if (((now_ms - b->last_edge_ms) >= BOUNCE_MS) && (raw != b->stable)) {
			b->stable = raw;
			if (b->stable == 1U) {
				b->pressed_state = 1U;
			}
		}
	}
}

uint8_t button_is_pressed(buttons_id button) {
	return s_button[button].stable;
}

uint8_t button_was_pressed(buttons_id button) {
	uint8_t event = s_button[button].pressed_state;
	s_button[button].pressed_state = 0U;
	return event;
}