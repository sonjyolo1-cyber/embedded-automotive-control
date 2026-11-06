#include "systick.h"
#include "stm32f10x.h"

static volatile uint32_t g_ticks;

void systick_init()
{
	SysTick->LOAD = 72000U - 1U;
	SysTick->VAL  = 0U;
	SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler()
{
	g_ticks++;
}

uint32_t milis()
{
	return g_ticks;
}