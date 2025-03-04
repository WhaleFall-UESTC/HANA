#include "riscv.h"
#include "platform.h"
#include "defs.h"

char init_stack[PGSIZE] __attribute__((aligned(PGSIZE)));
char *init_stack_top = &init_stack[PGSIZE];

char s[] = "Hello, World!\n";

int 
main()
{
    uart_init();
    
    for (int i = 0; i < 14; i++)
    {
        uart_putc(s[i]);
    }
    
}