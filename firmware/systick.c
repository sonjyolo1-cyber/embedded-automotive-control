#include "systick.h"

#define ST_CTRL_R 	(*((volatile uint32_t *)0xE000E010UL))
#define ST_RELOAD_R (*((volatile uint32_t *)0xE000E014UL))
#define ST_CURR_R 	(*((volatile uint32_t *)0xE000E018UL))
	
#define ST_CTRL_CLK_SRC (1UL << 2)
#define ST_CTRL_INTEN		(1UL << 1)
#define ST_CTRL_EN			(1UL << 0)

volatile uint32_t g_ticks = 0U;

void systick_init(uint32_t clock_frequency_hz) {
	ST_CTRL_R = 0UL;
	
	ST_RELOAD_R = ((clock_frequency_hz / 1000UL) - 1UL);
	
	ST_CURR_R = 0UL;
	
	ST_CTRL_R |= (ST_CTRL_CLK_SRC | ST_CTRL_INTEN | ST_CTRL_EN);
}

/* Called by the hardware through the vector table in startup_TM4C123.s, not from C. */
// cppcheck-suppress unusedFunction
void SysTick_Handler(void) {
	g_ticks++;
}
