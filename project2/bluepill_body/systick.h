#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_init();
void SysTick_Handler();
uint32_t milis();

#endif