#ifndef UART_H
#define UART_H

#include <stdint.h>

/* USART1 on PA9 (TX), 8N1, interrupt-driven transmit */
void uart_init(uint32_t pclk2_hz, uint32_t baud);

/* printf-style log. Never blocks.
 * Returns the number of bytes queued, or -1 if the whole line was dropped
 * because the TX buffer had no room. Do NOT call from an interrupt. */
int log_printf(const char *fmt, ...);

/* How many lines were dropped since boot */
uint32_t log_dropped(void);

#endif /* UART_H */
