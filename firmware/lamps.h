#ifndef LAMPS_H
#define LAMPS_H

/*
Lamp driver - drives the 4 turn-signal LEDS (+buzzer, stretch)
on Port E, per the pin map in design.md:

LED_FL (front-left) -> PE0
LED_RL (rear-left) -> PE1
LED_FR (front-right) -> PE2
LED_RR (rear-right) -> PE3
BUZZER (stretch) -> PE4
*/

#include <stdint.h>

typedef enum{
	LAMP_FRONT_LEFT = 0,
	LAMP_REAR_LEFT = 1,
	LAMP_FRONT_RIGHT = 2,
	LAMP_REAR_RIGHT = 3,
	LAMP_BUZZER = 4
}lamp_id_t;

void lamp_init(void);

void lamp_set(lamp_id_t lamp, uint8_t value);

void lamp_set_left(uint8_t value);
void lamp_set_right(uint8_t value);
void lamps_all_off(void);


#endif 