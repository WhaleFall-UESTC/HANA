#ifndef __UART_H__
#define __UART_H__

#include <irq/interrupt.h>

/* arch/<arch>/drivers/uart.c */
// uart 初始化设置
void            uart_init(void);
// 向 uart 无中断地写入字符，返回实际输出的字符
int             uart_putc_sync(int c);
// 将字符写入缓冲区，并告知 uart 发送，若 buffer 满会 slepp
void            uart_putc(int c);
// 从 uart 读取字符
int             uart_getc();
// uart 设备中断处理
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