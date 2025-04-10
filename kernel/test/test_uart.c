#include <common.h>

#ifdef ARCH_RISCV
#include <riscv.h>
#include <drivers/uart.h>
#endif

#include <klib.h>
#include <debug.h>

void test_uart() {
    char buf[] = "1234567890qwertyuiop[]\\asdfghjkl;'zxcvbnm,./";
    int len = sizeof(buf);
    for (int i = 0; i < len; i++)
        uart_putc(buf[i]);
}