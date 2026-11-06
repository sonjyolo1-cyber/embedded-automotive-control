/*
 * Project 2 - Blue Pill 1 (body / turn-signal node)
 * Session 2: clock 72 MHz + SysTick 1 ms + UART log with timestamps.
 */
#include "stm32f10x.h"
#include "clock.h"
#include "systick.h"
#include "uart.h"

/* On-board LED on PC13, active low. Used as a heartbeat so you can
 * see the board is alive even if the UART is not working. */
static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    /* CRH bits [23:20] for pin 13: CNF = 00 (push-pull), MODE = 10 (2 MHz) */
    GPIOC->CRH  = (GPIOC->CRH & ~(0xFu << 20)) | (0x2u << 20);
    GPIOC->BSRR = GPIO_BSRR_BS13;                      /* LED off */
}

static void led_toggle(void)
{
    GPIOC->ODR ^= GPIO_ODR_ODR13;
}

static const char *clock_name(clock_status_t cs)
{
    switch (cs) {
    case CLOCK_OK:      return "HSE+PLL";
    case CLOCK_ERR_HSE: return "HSI (HSE FAIL)";
    default:            return "HSI (PLL FAIL)";
    }
}

int main(void)
{
    clock_status_t cs = clock_init();
    uint32_t       last;

    systick_init(clock_sysclk_hz());
    uart_init(clock_pclk2_hz(), 115200u);
    led_init();

    log_printf("\r\n[%7lu ms] boot ok\r\n", (unsigned long)millis());
    log_printf("[%7lu ms] clock=%s SYSCLK=%lu PCLK1=%lu PCLK2=%lu\r\n",
               (unsigned long)millis(), clock_name(cs),
               (unsigned long)clock_sysclk_hz(),
               (unsigned long)clock_pclk1_hz(),
               (unsigned long)clock_pclk2_hz());

    last = millis();
    for (;;) {
        /* Subtraction keeps working when millis() wraps after ~49 days */
        if ((uint32_t)(millis() - last) >= 1000u) {
            last += 1000u;
            led_toggle();
            log_printf("[%7lu ms] tick dropped=%lu\r\n",
                       (unsigned long)millis(),
                       (unsigned long)log_dropped());
        }
    }
}
