#ifndef __UART_H__
#define __UART_H__

#include <irq/interrupt.h>

/* arch/riscv/drivers/uart.c */
void            uart_init(void);
int             uart_putc_sync(int c);
void            uart_putc(int c);
int             uart_getc();
irqret_t        uart_isr(uint32, void*);

typedef int (*putchar_t)(int);
extern putchar_t put_char;

static inline int putchar(int c) {
    return put_char(c);
}

#endif // __UART_H__