#include <common.h>

#include <arch.h>
#include <drivers/uart.h>

#include <klib.h>
#include <debug.h>

void test_uart() {
    char buf[] = "1234567890qwertyuiop[]\\asdfghjkl;'zxcvbnm,./";
    int len = sizeof(buf);
    for (int i = 0; i < len; i++)
        put_char(buf[i]);
}