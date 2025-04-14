#ifndef __UART_H__
#define __UART_H__

#include <irq/interrupt.h>

/* arch/riscv/drivers/uart.c */
void            uart_init(void);
int             uart_putc_sync(char c);
void            uart_putc(char c);
int             uart_getc();
irqret_t        uart_isr(uint32, void*);

static inline int
putchar(int c) 
{
    return uart_putc_sync(c);
}

#endif // __UART_H__