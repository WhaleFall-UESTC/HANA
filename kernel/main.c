#include <riscv.h>
#include <platform.h>
#include <defs.h>

#include <testdefs.h>

char init_stack[PGSIZE] __attribute__((aligned(PGSIZE)));
char *init_stack_top = &init_stack[PGSIZE];

int 
main()
{
    uart_init();
    puts("Initialize uart0\n");
    
    return 0;
}
