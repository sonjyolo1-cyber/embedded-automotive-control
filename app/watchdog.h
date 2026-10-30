#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdint.h>

/*
 * WDT0 watchdog (Phase 6). Feed it once per main loop pass, never from an ISR:
 * SysTick keeps running when main hangs, so feeding there would hide the hang.
 *
 * TM4C WDT0 resets on the SECOND time-out (the first only sets WDTRIS), so the
 * load value is half the wanted time-out.
 */

/* Start WDT0: reset after timeout_ms without watchdog_feed(). Cannot be stopped
 * until the next reset (INTEN is write-once). Stalls while a debugger halts the CPU. */
void watchdog_init(uint32_t clock_hz, uint32_t timeout_ms);

/* Proof that main finished one loop pass: clears the time-out flag, reloads. */
void watchdog_feed(void);

/* Reset cause (RESC) bits. */
#define RESET_CAUSE_PIN       (1UL << 0)   /* RESET button / external pin */
#define RESET_CAUSE_POWER_ON  (1UL << 1)
#define RESET_CAUSE_BROWN_OUT (1UL << 2)
#define RESET_CAUSE_WDT0      (1UL << 3)
#define RESET_CAUSE_SOFTWARE  (1UL << 4)

/* Read why the chip last reset, then clear RESC so the next reset reports a
 * fresh cause. Call first thing in main(). */
uint32_t watchdog_take_reset_cause(void);

#endif /* WATCHDOG_H */
