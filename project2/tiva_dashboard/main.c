#include "gpio.h"
#include "uart.h"
#include "can.h"

int main()
{
	extern uint32_t SystemCoreClock;
	extern void SystemCoreClockUpdate();
	uint32_t rx_id;
  uint8_t  rx_data;
	
	uart_init(SystemCoreClock, 115200U);
	uart_puts("Clock is: "); uart_put_uint(SystemCoreClock); uart_puts("Hz\r\n");
	
	can_init();
	uart_puts("CAN init ok\r\n");
	
	can_send(0x200, 1);
	while (!can_tx_ok()) {}
	uart_puts("tx ok\r\n");
		
	can_send(0x200, 1);
  while (!can_receive(&rx_id, &rx_data)) { }
	
	uart_puts("rx id=");
	uart_put_uint(rx_id);
	uart_puts(" data=");
	uart_put_uint(rx_data);
	uart_puts("\r\n");
	
	while (1)
	{
		
	}
	
	return 0;
}