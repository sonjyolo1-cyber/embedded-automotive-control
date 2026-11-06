#include "stm32f10x.h"
#include "uart.h"
#include "clock.h"
#include "systick.h"
#include "can.h"
#include <stdint.h>

int main()
{
	unsigned int rx_id;
	unsigned char rx_data;
	
	clock_init();
	uart_init();
	can_init();
	can_send(0x456U, 5);
	while (!can_receive(&rx_id, &rx_data)) {}
	uart_puts("rx id ="); uart_putnums(rx_id);
	uart_puts("\r\nrx data ="); uart_putnums(rx_data); uart_puts("\r\n");
	uart_puts("can init ok\r\n");
	systick_init();
	uint32_t now = 0;
	uart_puts("Hello\r\n");
	uart_puts("0919459034\r\n");
	
	while (1)
	{
		if (milis() - now >= 1000U)
		{
			now = now + 1000U;
			uart_puts("["); uart_putnums(now); uart_puts(" ms] tick\r\n");
		}
	}
	
	return 0;
	
}