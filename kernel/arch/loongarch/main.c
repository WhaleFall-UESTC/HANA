#include <common.h>
#include <klib.h>
#include <loongarch.h>
#include <debug.h>

typedef int (*putchar_t)(int);
extern putchar_t put_char;

// temporary stack for boot
char init_stack[KSTACK_SIZE * NCPU] __attribute__((aligned(PGSIZE)));

void uart_init(void);

int main() {
    uart_init();
    PASS("loongarch64 start!!!");
}