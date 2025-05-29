#ifndef __UART_H__
#define __UART_H__

#include <irq/interrupt.h>

/* arch/<arch>/drivers/uart.c */
void            uart_init(void);
int             uart_putc_sync(int c);
void            uart_putc(int c);
int             uart_getc();
int             uart_isr();

typedef int (*putchar_t)(int);
extern putchar_t put_char;
typedef int (*getchar_t)();
extern getchar_t get_char;

static inline int putchar(int c) {
    return put_char(c);
}
static inline int getchar() {
    return get_char();
}

#endif // __UART_H__