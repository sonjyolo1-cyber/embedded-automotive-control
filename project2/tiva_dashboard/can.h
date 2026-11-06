#ifndef CAN_H
#define CAN_H

#include <stdint.h>

void can_init();
void can_send(uint32_t id, uint8_t data);
int can_tx_ok(void);
int can_receive(uint32_t *id, uint8_t *data);

#endif