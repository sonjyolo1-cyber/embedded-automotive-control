#include <stddef.h>
#include "buzzer.h"

void buzzer_init(buzzer_t *bz, uint32_t now_ms)
{
    if (bz == NULL) {
        return;
    }
    bz->prev_lamp      = 0U;
    bz->pulse_start_ms = now_ms;
    bz->active         = 0U;
}

uint8_t buzzer_update(buzzer_t *bz, uint8_t lamp_on, uint32_t now_ms)
{
    uint8_t lamp;

    if (bz == NULL) {
        return 0U;                          /* safe default: coil off */
    }
    lamp = (lamp_on != 0U) ? 1U : 0U;       /* any non-zero means lit */

    /* 1. Lamp edge (on or off): start, or restart, the pulse. */
    if (lamp != bz->prev_lamp) {
        bz->prev_lamp      = lamp;
        bz->pulse_start_ms = now_ms;
        bz->active         = 1U;
    }

    /* 2. Pulse over? Unsigned subtraction stays correct across the 32-bit wrap. */
    if ((bz->active != 0U) &&
        ((uint32_t)(now_ms - bz->pulse_start_ms) >= BUZZER_PULSE_MS)) {
        bz->active = 0U;
    }

    return bz->active;
}
