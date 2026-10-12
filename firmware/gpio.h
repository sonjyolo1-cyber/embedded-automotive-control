#ifndef GPIO_H
#define GPIO_H

/*
Generic register-level GPIO helpers for the TM4C123, using the address-bus bit-masking scheme described in the datasheet 
(section 10.2.1.2, "Data Register Operation").

Every GPIO port exposes GPIODATA at 256 addresses (base + 0x000.. base + 0x3FC). Address bits [9:2] act as a write/read mask:
bit N of the offset (N  = 2...9) unlocks data bit (N-2).

To touch exactly one pin, use offset = 1 << (pin_num + 2).
*/

#include <stdint.h>

/* Port base addresses (APB aperture) */
#define GPIO_PORTA_BASE 0x40004000UL
#define GPIO_PORTB_BASE 0x40005000UL
#define GPIO_PORTC_BASE 0x40006000UL
#define GPIO_PORTD_BASE 0x40007000UL
#define GPIO_PORTE_BASE 0x40024000UL
#define GPIO_PORTF_BASE 0x40025000UL

/* Common per-port register offsets */
#define GPIO_O_DIR 			0x400UL
#define GPIO_O_AFSEL		0x420UL
#define GPIO_O_DR2R			0x500UL
#define GPIO_O_PUR			0x510UL
#define GPIO_O_DEN			0x51CUL
#define GPIO_O_AMSEL		0x528UL
#define GPIO_O_PCTL			0x52CUL

/* RCGCGPIO (clock gating control) - SYSCTL base 0x400FE000, offset 0x608 */
#define SYSCTL_RCGCGPIO_R (*((volatile uint32_t *) 0x400FE608))

/* PRGPIO (peripheral ready) - SYSCTL base 0x400FE000, offset 0xA08.
   Same bit order as RCGCGPIO; a bit reads 1 once that port is ready. */
#define SYSCTL_PRGPIO_R (*((volatile uint32_t *) 0x400FEA08))
	
/* Port F clock-enable bit position (used by the phase 0 example) */
#define SYSCTL_RCGCGPIO_PORTF (1UL << 5)

/* Map a pin number (0-7) to its RCGCGPIO-style enable bit for a port, per the datasheet's ports-order in the RCGCGPIO bit table:
	bit0 = A, bit1 = B, bit2 = C, bit3 = D, bit4 = E, bit5 = F
*/
#define SYSCTL_RCGCGPIO_PORTA (1UL << 0)
#define SYSCTL_RCGCGPIO_PORTB (1UL << 1)
#define SYSCTL_RCGCGPIO_PORTC (1UL << 2)
#define SYSCTL_RCGCGPIO_PORTD (1UL << 3)
#define SYSCTL_RCGCGPIO_PORTE (1UL << 4)

/*
 * gpio_enable_clock: enable a port's clock in RCGCGPIO, then wait in
 * PRGPIO until the port is ready. Accessing the port before it is ready
 * can cause a bus fault.
 */
static inline void gpio_enable_clock(uint32_t rcgc_bit)
{
	SYSCTL_RCGCGPIO_R |= rcgc_bit;

	/* Wait until the port is actually ready before touching its registers */
	while ((SYSCTL_PRGPIO_R & rcgc_bit) == 0U) {
	}
}

/*
 * gpio_set_dir: configure `pin_mask` bits of a port as outputs (1)
 * or inputs (0). pin_mask uses plain bit positions (bit0=pin0, etc),
 * NOT the masked-address encoding ? GPIODIR is a normal register.
 */
static inline void gpio_set_output(uint32_t port_base, uint8_t pin_mask)
{
	volatile uint32_t *dir = (volatile uint32_t *)(port_base + GPIO_O_DIR);
	*dir |= pin_mask;
}

static inline void gpio_set_input(uint32_t port_base, uint8_t pin_mask)
{
	volatile uint32_t *dir = (volatile uint32_t *)(port_base + GPIO_O_DIR);
	*dir &= ~((uint32_t)pin_mask);
}

/* gpio_enable_digital: set GPIOEN bits for 'pin_mask'.*/

static inline void gpio_enable_digital(uint32_t port_base, uint8_t pin_mask)
{
	volatile uint32_t *den = (volatile uint32_t *)(port_base + GPIO_O_DEN);
	*den |= pin_mask;
}

/* gpio_enable_pullup: set GPIOPUR bits for 'pin_mask' (inputs only). */
static inline void gpio_enable_pullup(uint32_t port_base, uint8_t pin_mask)
{
	volatile uint32_t *pur = (volatile uint32_t *)(port_base + GPIO_O_PUR);
	*pur |= pin_mask;
}


/*
 * gpio_write_pin: write a single pin through the masked-address
 * GPIODATA aperture, touching only that one pin.
 *
 *   pin_num : 0-7
 *   value   : 0 or nonzero (nonzero -> pin driven high)
 *
 * This is the "single instruction, only this pin" technique from
 * the datasheet ? no read-modify-write needed at the call site.
 */

static inline void gpio_write_pin(uint32_t port_base, uint8_t pin_num, uint8_t value)
{
	uint32_t offset = 1UL << (pin_num + 2);
	volatile uint32_t *data = (volatile uint32_t *)(port_base + offset);
	*data = value ? 0xFFU : 0x00U;
}

/*
 * gpio_read_pin: read a single pin through the same masked aperture.
 * Returns 0 or 1.
 */
static inline uint8_t gpio_read_pin(uint32_t port_base, uint8_t pin_num)
{
    uint32_t offset = 1UL << (pin_num + 2);
    volatile uint32_t *data = (volatile uint32_t *)(port_base + offset);
    return (*data != 0) ? 1U : 0U;
}


#endif