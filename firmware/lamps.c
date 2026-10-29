#include "gpio.h"
#include "lamps.h"

#define LAMPS_PORT_BASE GPIO_PORTE_BASE
#define LAMPS_PIN_MASK 0x1FU /* PE0...PE4 -> bits 0...4 */

void lamp_init(void){
	gpio_enable_clock(SYSCTL_RCGCGPIO_PORTE);
	
	gpio_set_output(LAMPS_PORT_BASE, LAMPS_PIN_MASK);
	
	gpio_enable_digital(LAMPS_PORT_BASE, LAMPS_PIN_MASK);
	
	lamps_all_off();
}

void lamp_set(lamp_id_t lamp, uint8_t value){
	gpio_write_pin(LAMPS_PORT_BASE, (uint8_t)lamp, value);
}

void lamp_set_left(uint8_t value) {
	lamp_set(LAMP_FRONT_LEFT, value);
	lamp_set(LAMP_REAR_LEFT, value);
}

void lamp_set_right(uint8_t value) {
	lamp_set(LAMP_FRONT_RIGHT, value);
	lamp_set(LAMP_REAR_RIGHT, value);
}

void lamps_all_off(void) {
	lamp_set(LAMP_FRONT_LEFT, 0);
	lamp_set(LAMP_REAR_LEFT, 0);
	lamp_set(LAMP_FRONT_RIGHT, 0);
	lamp_set(LAMP_REAR_RIGHT, 0);
	lamp_set(LAMP_BUZZER, 0);
}