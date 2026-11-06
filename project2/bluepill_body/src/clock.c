/*
 * clock.c - SYSCLK 72 MHz from the 8 MHz HSE crystal through the PLL.
 * Reference: RM0008, section 7.2 (clocks) and 7.3 (RCC registers),
 *            section 3.3.3 (FLASH_ACR, wait states).
 *
 *   HSE 8 MHz --> PLL x9 --> SYSCLK 72 MHz --> AHB /1 = 72 MHz
 *                                                 |--> APB2 /1 = 72 MHz (USART1, GPIO)
 *                                                 |--> APB1 /2 = 36 MHz (bxCAN, TIM2-4)
 */
#include "stm32f10x.h"
#include "clock.h"

#define HSE_TIMEOUT  100000u   /* loop count, roughly 100 ms at 8 MHz */
#define PLL_TIMEOUT  100000u

/* After reset the chip runs on HSI 8 MHz with all prescalers = 1 */
static uint32_t s_sysclk_hz = 8000000u;
static uint32_t s_pclk1_hz  = 8000000u;
static uint32_t s_pclk2_hz  = 8000000u;

/* Wait until (reg & mask) == expected. Returns 1 on success, 0 on timeout. */
static int wait_flag(volatile uint32_t *reg, uint32_t mask,
                     uint32_t expected, uint32_t timeout)
{
    while (((*reg) & mask) != expected) {
        if (timeout == 0u) {
            return 0;
        }
        timeout--;
    }
    return 1;
}

clock_status_t clock_init(void)
{
    uint32_t cfgr;

    /* 1. Start the external 8 MHz crystal and wait until it is stable */
    RCC->CR |= RCC_CR_HSEON;
    if (!wait_flag(&RCC->CR, RCC_CR_HSERDY, RCC_CR_HSERDY, HSE_TIMEOUT)) {
        RCC->CR &= ~RCC_CR_HSEON;
        return CLOCK_ERR_HSE;                 /* stay on HSI 8 MHz */
    }

    /* 2. Flash needs 2 wait states when 48 MHz < SYSCLK <= 72 MHz.
     *    Must be set BEFORE switching to the faster clock. */
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    /* 3. Bus prescalers and PLL settings (PLL must still be off) */
    cfgr  = RCC->CFGR;
    cfgr &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 |
              RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
    cfgr |=  RCC_CFGR_HPRE_DIV1               /* AHB  = 72 MHz           */
          |  RCC_CFGR_PPRE1_DIV2              /* APB1 = 36 MHz (max 36)  */
          |  RCC_CFGR_PPRE2_DIV1              /* APB2 = 72 MHz           */
          |  RCC_CFGR_PLLSRC_HSE              /* PLL input = HSE, not /2 */
          |  RCC_CFGR_PLLMULL9;               /* 8 MHz x 9 = 72 MHz      */
    RCC->CFGR = cfgr;

    /* 4. Turn the PLL on and wait for lock */
    RCC->CR |= RCC_CR_PLLON;
    if (!wait_flag(&RCC->CR, RCC_CR_PLLRDY, RCC_CR_PLLRDY, PLL_TIMEOUT)) {
        RCC->CR  &= ~(RCC_CR_PLLON | RCC_CR_HSEON);
        RCC->CFGR = 0u;                       /* back to reset prescalers */
        return CLOCK_ERR_PLL;                 /* still on HSI 8 MHz */
    }

    /* 5. Switch SYSCLK to the PLL and confirm with the status bits */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    if (!wait_flag(&RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_PLL, PLL_TIMEOUT)) {
        RCC->CR  &= ~(RCC_CR_PLLON | RCC_CR_HSEON);
        RCC->CFGR = 0u;
        return CLOCK_ERR_PLL;
    }

    s_sysclk_hz = 72000000u;
    s_pclk1_hz  = 36000000u;
    s_pclk2_hz  = 72000000u;
    return CLOCK_OK;
}

uint32_t clock_sysclk_hz(void) { return s_sysclk_hz; }
uint32_t clock_pclk1_hz(void)  { return s_pclk1_hz;  }
uint32_t clock_pclk2_hz(void)  { return s_pclk2_hz;  }
