#include "uart.h"
#include "stm32f10x.h"

#define SIZE 256

static char queue[SIZE];
static int write_index;
static int read_index;
static volatile int count;


void uart_init()
{
	RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN);
	
	GPIOA->CRH &= ~(0xFU << 4);
	GPIOA->CRH |= (0xBU << 4);
	
	USART1->BRR = 625;
	USART1->CR1 = (USART_CR1_UE | USART_CR1_TE);
	
	NVIC_EnableIRQ(USART1_IRQn);
}

void uart_putc(char c)
{
	if (count == SIZE)
	{
		return;
	}
	queue[write_index++] = c;
	if (write_index == SIZE) write_index = 0;
	__disable_irq();
	count++;
	__enable_irq();
	USART1->CR1 |= USART_CR1_TXEIE;
}

void USART1_IRQHandler(void) 
{
	if (count == 0U)
	{
		USART1->CR1 &= ~(USART_CR1_TXEIE);
		return;
	}
	USART1->DR = queue[read_index++];
	if (read_index == SIZE) read_index = 0;
	count--;
}

void uart_puts(const char *s)
{
	while (*s)
	{
		uart_putc(*s++);
	}
}

void uart_putnums(unsigned int n)
{
	char num[10];
	int index = 0;
	if (n == 0)
	{
		uart_putc('0');
		return;
	}
	while (n > 0)
	{
		num[index] = '0' + (n % 10);
		n = n / 10;
		index++;
	}
	while (index > 0)
	{
		index--;
		uart_putc(num[index]);
	}
	
}