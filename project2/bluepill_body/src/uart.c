/*
 * uart.c - interrupt-driven USART1 transmit with a ring buffer.
 * Reference: RM0008, chapter 27 (USART), section 9.2 (GPIO CRL/CRH).
 *
 *   log_printf() --writes--> [ s_buf, 256 B ] --reads--> USART1_IRQHandler --> PA9
 *                 moves s_head                 moves s_tail
 *
 * Each index has exactly one writer, so no lock is needed:
 *   s_head is written only by log_printf() (main loop)
 *   s_tail is written only by USART1_IRQHandler()
 */
#include <stdarg.h>
#include <stdio.h>
#include "stm32f10x.h"
#include "uart.h"

#define TX_BUF_SIZE  256u                  /* must be a power of 2 */
#define TX_BUF_MASK  (TX_BUF_SIZE - 1u)
#define LINE_MAX     96u                   /* longest single log line */

static uint8_t           s_buf[TX_BUF_SIZE];
static volatile uint16_t s_head    = 0u;
static volatile uint16_t s_tail    = 0u;
static volatile uint32_t s_dropped = 0u;

void uart_init(uint32_t pclk2_hz, uint32_t baud)
{
    /* Clocks for GPIOA and USART1 (both on APB2) */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* PA9 = USART1_TX: alternate-function push-pull, 50 MHz
     * CRH bits [7:4] for pin 9: CNF = 10, MODE = 11 -> 0xB */
    GPIOA->CRH = (GPIOA->CRH & ~(0xFu << 4)) | (0xBu << 4);
    /* PA10 = USART1_RX stays input floating (reset state); not used yet */

    USART1->CR1 = 0u;                                  /* disable while configuring */
    USART1->BRR = (uint16_t)((pclk2_hz + baud / 2u) / baud);   /* 72e6 / 115200 = 625 */
    USART1->CR2 = 0u;                                  /* 1 stop bit */
    USART1->CR3 = 0u;                                  /* no flow control */
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE;         /* 8N1, TX on, TXEIE off */

    NVIC_EnableIRQ(USART1_IRQn);
}

/* Bytes that can still be queued. One slot is kept empty so that
 * head == tail always means "empty", never "full". */
static uint16_t free_space(void)
{
    uint16_t used = (uint16_t)((s_head - s_tail) & TX_BUF_MASK);
    return (uint16_t)(TX_BUF_SIZE - 1u - used);
}

int log_printf(const char *fmt, ...)
{
    char     line[LINE_MAX];
    va_list  ap;
    int      len;
    int      i;
    uint16_t head;

    va_start(ap, fmt);
    len = vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);

    if (len <= 0) {
        return 0;
    }
    if (len >= (int)sizeof line) {
        len = (int)sizeof line - 1;                    /* line was truncated */
    }

    /* Not enough room for the WHOLE line: drop it and count */
    if ((uint16_t)len > free_space()) {
        s_dropped++;
        return -1;
    }

    head = s_head;
    for (i = 0; i < len; i++) {
        s_buf[head] = (uint8_t)line[i];
        head = (uint16_t)((head + 1u) & TX_BUF_MASK);
    }

    __DMB();                    /* make sure the bytes are stored before head moves */
    s_head = head;              /* publish the new data to the ISR */

    USART1->CR1 |= USART_CR1_TXEIE;                    /* start / keep sending */
    return len;
}

void USART1_IRQHandler(void)
{
    if ((USART1->SR & USART_SR_TXE) && (USART1->CR1 & USART_CR1_TXEIE)) {
        uint16_t tail = s_tail;

        if (tail != s_head) {
            USART1->DR = s_buf[tail];                  /* also clears TXE */
            s_tail = (uint16_t)((tail + 1u) & TX_BUF_MASK);
        } else {
            USART1->CR1 &= (uint16_t)~USART_CR1_TXEIE; /* buffer empty: stop */
        }
    }
}

uint32_t log_dropped(void)
{
    return s_dropped;
}
