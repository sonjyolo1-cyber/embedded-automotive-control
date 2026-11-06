#include "stm32f10x.h"
#include "can.h"

void can_init()
{
	RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
	
	CAN1->MCR &= ~(CAN_MCR_SLEEP);
	CAN1->MCR |= CAN_MCR_INRQ;
	while (!(CAN1->MSR & CAN_MSR_INAK)) {}
	
	CAN1->BTR = CAN_BTR_LBKM | (1U << 20) | (14U << 16) | 3U;
	
	CAN1->MCR &= ~CAN_MCR_INRQ;
	while (CAN1->MSR & CAN_MSR_INAK) {}
		
	CAN1->FMR |= CAN_FMR_FINIT;
	CAN1->FS1R |= CAN_FS1R_FSC0;
	CAN1->sFilterRegister[0].FR1 = 0;
	CAN1->sFilterRegister[0].FR2 = 0;
	CAN1->FA1R |= CAN_FA1R_FACT0;
	CAN1->FMR &= ~CAN_FMR_FINIT;
}

void can_send(unsigned int id, unsigned char data)
{
	while (!(CAN1->TSR & CAN_TSR_TME0)) {}
		
	CAN1->sTxMailBox[0].TDTR = 1;
	CAN1->sTxMailBox[0].TDLR = data;
		
	CAN1->sTxMailBox[0].TIR = (id << 21) | CAN_TI0R_TXRQ;
}

int can_receive(unsigned int *id, unsigned char *data)
{
	if ((CAN1->RF0R & CAN_RF0R_FMP0) == 0)
	{
		return 0;
	}
	
	*id = CAN1->sFIFOMailBox[0].RIR >> 21;
	*data = CAN1->sFIFOMailBox[0].RDLR & 0xFF;
	
	CAN1->RF0R |= CAN_RF0R_RFOM0;
	return 1;
}