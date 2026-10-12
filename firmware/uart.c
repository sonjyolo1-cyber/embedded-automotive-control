#include "gpio.h"
#include "uart.h"

/* ---- System Control: UART clock gating (datasheet ch. 5) ---- */
#define SYSCTL_RCGCUART_R   (*((volatile uint32_t *)0x400FE618UL))  /* clock enable */
#define SYSCTL_PRUART_R     (*((volatile uint32_t *)0x400FEA18UL))  /* peripheral ready */
#define SYSCTL_UART0        (1UL << 0)

/* ---- UART0 registers, base 0x4000C000 (datasheet ch. 14) ---- */
#define UART0_BASE          0x4000C000UL
#define UART0_DR_R    (*((volatile uint32_t *)(UART0_BASE + 0x000UL)))  /* data          */
#define UART0_FR_R    (*((volatile uint32_t *)(UART0_BASE + 0x018UL)))  /* flags         */
#define UART0_IBRD_R  (*((volatile uint32_t *)(UART0_BASE + 0x024UL)))  /* baud, integer */
#define UART0_FBRD_R  (*((volatile uint32_t *)(UART0_BASE + 0x028UL)))  /* baud, 1/64ths */
#define UART0_LCRH_R  (*((volatile uint32_t *)(UART0_BASE + 0x02CUL)))  /* line control  */
#define UART0_CTL_R   (*((volatile uint32_t *)(UART0_BASE + 0x030UL)))  /* control       */
#define UART0_CC_R    (*((volatile uint32_t *)(UART0_BASE + 0xFC8UL)))  /* clock source  */

#define UART_FR_TXFF        (1UL << 5)    /* TX FIFO full                     */
#define UART_FR_BUSY        (1UL << 3)    /* still shifting bits out          */
#define UART_LCRH_WLEN_8    (0x3UL << 5)  /* 8 data bits (N and 1 are default) */
#define UART_LCRH_FEN       (1UL << 4)    /* FIFO enable                      */
#define UART_CTL_UARTEN     (1UL << 0)
#define UART_CTL_TXE        (1UL << 8)
#define UART_CTL_RXE        (1UL << 9)
#define UART_CC_SYSCLK      0x0UL         /* baud clock = system clock        */

/* ---- PA0/PA1 handed from GPIO to UART0 ---- */
#define UART_PINS           0x03U         /* PA0, PA1 -> bits 0..1            */
#define PCTL_PA0_PA1_MASK   0x000000FFUL  /* PMC0 = bits 3:0, PMC1 = bits 7:4 */
#define PCTL_PA0_PA1_UART   0x00000011UL  /* function 1 on both = U0RX/U0TX  */

void uart_init(uint32_t clock_hz, uint32_t baud)
{
    volatile uint32_t *afsel = (volatile uint32_t *)(GPIO_PORTA_BASE + GPIO_O_AFSEL);
    volatile uint32_t *pctl  = (volatile uint32_t *)(GPIO_PORTA_BASE + GPIO_O_PCTL);
    volatile uint32_t *amsel = (volatile uint32_t *)(GPIO_PORTA_BASE + GPIO_O_AMSEL);

    /* 1. Clocks: UART0 and GPIO Port A, wait until both are ready. */
    SYSCTL_RCGCUART_R |= SYSCTL_UART0;
    while ((SYSCTL_PRUART_R & SYSCTL_UART0) == 0U) {
    }
    gpio_enable_clock(SYSCTL_RCGCGPIO_PORTA);        /* also waits on PRGPIO */

    /* 2. Hand PA0/PA1 to the UART: alternate function + which function. */
    *afsel |= UART_PINS;                              /* AFSEL: not plain GPIO   */
    *pctl   = (*pctl & ~PCTL_PA0_PA1_MASK) | PCTL_PA0_PA1_UART;  /* PCTL: = UART */
    *amsel &= ~(uint32_t)UART_PINS;                   /* no analog function      */
    gpio_enable_digital(GPIO_PORTA_BASE, UART_PINS);  /* DEN                     */

    /* 3. Disable the UART while configuring it. */
    UART0_CTL_R &= ~UART_CTL_UARTEN;

    /* 4. Baud divisor, in 1/64ths, rounded, integer math only:
     *      BRD x 64 = clock / (16 x baud) x 64 = clock x 4 / baud
     *    50 MHz, 115200: 200000000 / 115200 = 1736.1 -> 1736
     *      IBRD = 1736 / 64 = 27,  FBRD = 1736 % 64 = 8               */
    uint32_t brd_x64 = ((clock_hz * 4U) + (baud / 2U)) / baud;
    UART0_IBRD_R = brd_x64 >> 6;       /* integer part     */
    UART0_FBRD_R = brd_x64 & 0x3FU;    /* fraction, 6 bits */

    /* 5. 8N1 + FIFO. Must come AFTER IBRD/FBRD: this write latches them. */
    UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN;

    /* 6. Baud clock from the system clock, then enable TX, RX and the UART. */
    UART0_CC_R  = UART_CC_SYSCLK;
    UART0_CTL_R = UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE;
}

void uart_putc(char c)
{
    while ((UART0_FR_R & UART_FR_TXFF) != 0U) {   /* wait only if FIFO is full */
    }
    UART0_DR_R = (uint32_t)(uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s != '\0') {
        uart_putc(*s);
        s++;
    }
}

void uart_put_uint(uint32_t value)
{
    char digits[10];        /* 4294967295 has 10 digits */
    uint8_t n = 0U;

    /* Peel off digits from the right: 1234 -> '4','3','2','1' */
    do {
        digits[n] = (char)('0' + (value % 10U));
        value /= 10U;
        n++;
    } while (value != 0U);

    /* Send them back in the right order. */
    while (n > 0U) {
        n--;
        uart_putc(digits[n]);
    }
}