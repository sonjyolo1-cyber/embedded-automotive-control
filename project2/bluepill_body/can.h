#ifndef CAN_H
#define CAN_H

void can_init();
void can_send(unsigned int id, unsigned char data);
int can_receive(unsigned int *id, unsigned char *data);

#endif