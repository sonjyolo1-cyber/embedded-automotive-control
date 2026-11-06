#ifndef UART_H
#define UART_H

#define SIZE 256

void uart_init();
void uart_putc(char c);
void USART1_IRQHandler(void);
void uart_puts(const char *s);
void uart_putnums(unsigned int n);

#endif