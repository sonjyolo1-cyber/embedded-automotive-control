#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

extern volatile uint32_t g_ticks;

void systick_init(uint32_t clock_frequency_hz);

#endif