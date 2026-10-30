#include "watchdog.h"

/* System control (datasheet 5.5) */
#define SYSCTL_RESC     (*((volatile uint32_t *)0x400FE05CUL))   /* reset cause        */
#define SYSCTL_RCGCWD   (*((volatile uint32_t *)0x400FE600UL))   /* watchdog clock gate */
#define SYSCTL_PRWD     (*((volatile uint32_t *)0x400FEA00UL))   /* watchdog ready      */
#define RCGCWD_R0       (1UL << 0)                               /* bit 0 = WDT0        */

/* Watchdog Timer 0 (datasheet 11.5) */
#define WDT0_BASE       0x40000000UL
#define WDT0_LOAD       (*((volatile uint32_t *)(WDT0_BASE + 0x000UL)))
#define WDT0_CTL        (*((volatile uint32_t *)(WDT0_BASE + 0x008UL)))
#define WDT0_ICR        (*((volatile uint32_t *)(WDT0_BASE + 0x00CUL)))
#define WDT0_TEST       (*((volatile uint32_t *)(WDT0_BASE + 0x418UL)))

#define WDTCTL_INTEN    (1UL << 0)   /* starts the counter; write-once until reset */
#define WDTCTL_RESEN    (1UL << 1)   /* second time-out resets the chip           */
#define WDTTEST_STALL   (1UL << 8)   /* stop counting while the debugger halts    */

void watchdog_init(uint32_t clock_hz, uint32_t timeout_ms)
{
    uint32_t load;

    /* Half the time-out, because the reset comes on the second time-out.
     * Divide first so 50 MHz x 500 ms stays far below 2^32. */
    load = (clock_hz / 1000UL) * timeout_ms / 2UL;

    SYSCTL_RCGCWD |= RCGCWD_R0;
    while ((SYSCTL_PRWD & RCGCWD_R0) == 0U) { }

    WDT0_LOAD  = load;
    WDT0_TEST |= WDTTEST_STALL;
    WDT0_CTL  |= WDTCTL_RESEN;     /* arm the reset first ...          */
    WDT0_CTL  |= WDTCTL_INTEN;     /* ... then start counting.         */
    /* The WDT interrupt is NOT enabled in the NVIC: the first time-out only
     * sets the flag, no ISR runs. WDTLOCK is left unlocked on purpose. */
}

void watchdog_feed(void)
{
    WDT0_ICR = 1U;                 /* any write: clear flag + reload WDT0_LOAD */
}

uint32_t watchdog_take_reset_cause(void)
{
    uint32_t cause = SYSCTL_RESC;
    SYSCTL_RESC = 0U;              /* bits are sticky: clear for the next reset */
    return cause;
}
