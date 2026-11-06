#ifndef UART_H
#define UART_H

/*
 * UART0 driver, transmit only, polling.
 *
 * Pins: PA0 = U0RX, PA1 = U0TX. On the LaunchPad these go to the on-board
 * debug chip, which shows up on the PC as "Stellaris Virtual Serial Port".
 * No extra wires needed.
 *
 * Format: 8N1 (8 data bits, no parity, 1 stop bit), TX FIFO enabled (16 bytes).
 *
 * Design note (design.md): polling is fine while each log line fits the
 * 16-byte FIFO (~1.2 ms on the wire at 115200). Switch to interrupt + ring
 * buffer if a single print gets longer than ~50 chars (~4 ms of waiting).
 */

#include <stdint.h>

/* clock_hz: pass SystemCoreClock, so the divisor follows the real clock. */
void uart_init(uint32_t clock_hz, uint32_t baud);

/* Send one character. Waits only if the 16-byte TX FIFO is full. */
void uart_putc(char c);

/* Send a 0-terminated string. Use "\r\n" for a new line in PuTTY. */
void uart_puts(const char *s);

/* Send an unsigned number in decimal, e.g. 1234 -> "1234". */
void uart_put_uint(uint32_t value);

#endif /* UART_H */