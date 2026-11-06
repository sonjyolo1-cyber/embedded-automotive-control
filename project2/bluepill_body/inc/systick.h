#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void     systick_init(uint32_t core_hz);   /* 1 ms interrupt */
uint32_t millis(void);                     /* ms since systick_init() */

#endif /* SYSTICK_H */
