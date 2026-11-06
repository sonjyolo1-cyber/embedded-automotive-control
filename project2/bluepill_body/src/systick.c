/*
 * systick.c - 1 ms time base.
 * Reference: PM0056 (Cortex-M3 programming manual), section 4.5 SysTick.
 */
#include "stm32f10x.h"
#include "systick.h"

static volatile uint32_t g_ticks = 0u;

void systick_init(uint32_t core_hz)
{
    SysTick->CTRL = 0u;                        /* stop while configuring */
    SysTick->LOAD = (core_hz / 1000u) - 1u;    /* 72 MHz -> 71999 -> 1 ms */
    SysTick->VAL  = 0u;                        /* clear current value */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk /* count on the CPU clock */
                  | SysTick_CTRL_TICKINT_Msk   /* interrupt at 0 */
                  | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void)
{
    g_ticks++;
}

uint32_t millis(void)
{
    /* A 32-bit aligned read is a single instruction on Cortex-M3,
     * so it cannot be torn by the interrupt. */
    return g_ticks;
}
