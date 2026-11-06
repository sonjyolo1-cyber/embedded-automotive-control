#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

typedef enum {
    CLOCK_OK = 0,      /* SYSCLK = 72 MHz from HSE + PLL */
    CLOCK_ERR_HSE,     /* HSE did not start -> still on HSI 8 MHz */
    CLOCK_ERR_PLL      /* PLL did not lock  -> back on HSI 8 MHz  */
} clock_status_t;

clock_status_t clock_init(void);

uint32_t clock_sysclk_hz(void);
uint32_t clock_pclk1_hz(void);   /* APB1: bxCAN, TIM2-4 */
uint32_t clock_pclk2_hz(void);   /* APB2: USART1, GPIO  */

#endif /* CLOCK_H */
