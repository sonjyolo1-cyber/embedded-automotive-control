#include "can.h"
#include <stdint.h>

#define SYSCTL_RCGCCAN_R  (*((volatile uint32_t *) 0x400FE634))
#define SYSCTL_PRCAN_R	  (*((volatile uint32_t *) 0x400FEA34))
#define SYSCTL_CAN0			  (1UL << 0)

#define CAN0_STS_R      (*((volatile uint32_t *)(CAN0_BASE + 0x004UL)))
#define CAN_STS_TXOK    (1UL << 3)    

#define CAN0_BASE 				0x40040000
#define CAN0_CTL_R			  (*((volatile uint32_t *)(CAN0_BASE + 0x000UL)))		 
#define CAN0_BIT_R 			  (*((volatile uint32_t *)(CAN0_BASE + 0x00CUL)))
#define CAN0_TST_R   		  (*((volatile uint32_t *)(CAN0_BASE + 0x014UL)))
#define CAN0_BRPE_R  		  (*((volatile uint32_t *)(CAN0_BASE + 0x018UL)))

#define CAN0_IF1CRQ_R   (*((volatile uint32_t *)(CAN0_BASE + 0x020UL)))
#define CAN0_IF1CMSK_R  (*((volatile uint32_t *)(CAN0_BASE + 0x024UL)))
#define CAN0_IF1ARB2_R  (*((volatile uint32_t *)(CAN0_BASE + 0x034UL)))
#define CAN0_IF1MCTL_R  (*((volatile uint32_t *)(CAN0_BASE + 0x038UL)))
#define CAN0_IF1DA1_R   (*((volatile uint32_t *)(CAN0_BASE + 0x03CUL)))

#define CAN0_IF2CRQ_R   (*((volatile uint32_t *)(CAN0_BASE + 0x080UL)))  
#define CAN0_IF2CMSK_R  (*((volatile uint32_t *)(CAN0_BASE + 0x084UL)))  
#define CAN0_IF2MSK2_R  (*((volatile uint32_t *)(CAN0_BASE + 0x08CUL)))  
#define CAN0_IF2ARB2_R  (*((volatile uint32_t *)(CAN0_BASE + 0x094UL)))  
#define CAN0_IF2MCTL_R  (*((volatile uint32_t *)(CAN0_BASE + 0x098UL)))  
#define CAN0_IF2DA1_R   (*((volatile uint32_t *)(CAN0_BASE + 0x04CUL)))  

#define CAN_CRQ_BUSY      (1UL << 15)  
#define CAN_CMSK_WRNRD    (1UL << 7)   
#define CAN_CMSK_ARB      (1UL << 5)   
#define CAN_CMSK_CONTROL  (1UL << 4)   
#define CAN_CMSK_DATAA    (1UL << 1)   
#define CAN_ARB2_MSGVAL   (1UL << 15)  
#define CAN_ARB2_DIR      (1UL << 13)  
#define CAN_MCTL_TXRQST   (1UL << 8)   
#define CAN_MCTL_EOB      (1UL << 7)   

#define CAN0_NWDA1_R    (*((volatile uint32_t *)(CAN0_BASE + 0x120UL)))

#define CAN_CMSK_MASK     (1UL << 6)    
#define CAN_CMSK_NEWDAT   (1UL << 2)    
#define CAN_MCTL_UMASK    (1UL << 12)   

#define CAN0_CTL_INIT		  (1UL << 0)
#define CAN0_CTL_CCE      (1UL << 6)
#define CAN_CTL_TEST   	  (1UL << 7)
#define CAN_TST_LBACK  	  (1UL << 4)


void can_init()
{
	SYSCTL_RCGCCAN_R |= SYSCTL_CAN0;
	while ((SYSCTL_PRCAN_R & SYSCTL_CAN0) == 0U) {}
		
	CAN0_CTL_R |= CAN0_CTL_INIT | CAN0_CTL_CCE;
		
	CAN0_BIT_R  = 0x2F04UL;
	CAN0_BRPE_R = 0UL;

	CAN0_CTL_R |= CAN_CTL_TEST;
	CAN0_TST_R |= CAN_TST_LBACK;
		
	CAN0_CTL_R &= ~(CAN0_CTL_INIT | CAN0_CTL_CCE);
		
	while ((CAN0_IF2CRQ_R & CAN_CRQ_BUSY) != 0U) {}
	
	CAN0_IF2CMSK_R = CAN_CMSK_WRNRD | CAN_CMSK_MASK | CAN_CMSK_ARB | CAN_CMSK_CONTROL;      
  CAN0_IF2MSK2_R = 0;                                    
  CAN0_IF2ARB2_R = CAN_ARB2_MSGVAL;                      
  CAN0_IF2MCTL_R = CAN_MCTL_UMASK | CAN_MCTL_EOB;
	CAN0_IF2CRQ_R = 2UL;
	while ((CAN0_IF2CRQ_R & CAN_CRQ_BUSY) != 0U) {}
}

void can_send(uint32_t id, uint8_t data)
{
	while ((CAN0_IF1CRQ_R & CAN_CRQ_BUSY) != 0U) {}
		
  CAN0_IF1CMSK_R = CAN_CMSK_WRNRD | CAN_CMSK_ARB | CAN_CMSK_CONTROL | CAN_CMSK_DATAA;
  CAN0_IF1ARB2_R = CAN_ARB2_MSGVAL | CAN_ARB2_DIR | (id << 2);
  CAN0_IF1MCTL_R = CAN_MCTL_TXRQST | CAN_MCTL_EOB | 1UL;   /* 1UL = DLC */
  CAN0_IF1DA1_R  = data;
		
	CAN0_IF1CRQ_R = 1UL;
	while ((CAN0_IF1CRQ_R & CAN_CRQ_BUSY) != 0U) {}
}

int can_tx_ok(void)
{
	return (CAN0_STS_R & CAN_STS_TXOK) != 0;
}

int can_receive(uint32_t *id, uint8_t *data)
{
	if ((CAN0_NWDA1_R & (1UL << 1)) == 0) {return 0;}
	
	while ((CAN0_IF2CRQ_R & CAN_CRQ_BUSY) != 0U) {}
		
	CAN0_IF2CMSK_R = CAN_CMSK_ARB | CAN_CMSK_CONTROL | CAN_CMSK_DATAA | CAN_CMSK_NEWDAT;
	CAN0_IF2CRQ_R = 2UL;
	while ((CAN0_IF2CRQ_R & CAN_CRQ_BUSY) != 0U) {}

	*id   = (CAN0_IF2ARB2_R >> 2) & 0x7FFUL;      
  *data = (uint8_t)(CAN0_IF2DA1_R & 0xFFUL);    
  return 1;
	
}
