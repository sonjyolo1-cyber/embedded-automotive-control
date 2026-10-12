#include "gpio.h"
#include "lamps.h"
#include "systick.h"
#include "buttons.h"
#include "uart.h"

extern uint32_t SystemCoreClock;
extern void SystemCoreClockUpdate();

/*
 * Phase 3: debounced buttons + UART log.
 * Bounce measurement removed: DEBOUNCE 20 ms chosen from data (max 4-5 ms).
 */
volatile uint32_t g_events_left   = 0U;
volatile uint32_t g_events_right  = 0U;
volatile uint32_t g_events_hazard = 0U;

/* Print "<name> pressed n=<count>\r\n". Longest line ~21 chars. */
static void log_press(const char *name, uint32_t count)
{
	uart_puts(name);
	uart_puts(" pressed n=");
	uart_put_uint(count);
	uart_puts("\r\n");
}

int main(){
	SystemCoreClockUpdate();
	uart_init(SystemCoreClock, 115200U);
	lamp_init();
	buttons_init();
	systick_init(SystemCoreClock);

	uart_puts("\r\n--- Phase 3: buttons + UART ---\r\n");
	uart_puts("SystemCoreClock = ");
	uart_put_uint(SystemCoreClock);
	uart_puts(" Hz\r\n");

	while(1) {
		uint32_t now = g_ticks;

		buttons_update(now);

		if (button_was_pressed(BUTTON_LEFT)) {
			g_events_left++;
			log_press("LEFT", g_events_left);
		}
		if (button_was_pressed(BUTTON_RIGHT)) {
			g_events_right++;
			log_press("RIGHT", g_events_right);
		}
		if (button_was_pressed(BUTTON_HAZARD)) {
			g_events_hazard++;
			log_press("HAZARD", g_events_hazard);
		}

		lamp_set_left(button_is_pressed(BUTTON_LEFT));
		lamp_set_right(button_is_pressed(BUTTON_RIGHT));
	}
}