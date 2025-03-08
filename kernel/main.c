#include <riscv.h>
#include <platform.h>
#include <memlayout.h>
#include <defs.h>

#include <testdefs.h>

char init_stack[PGSIZE] __attribute__((aligned(PGSIZE)));
char *init_stack_top = &init_stack[PGSIZE];

extern char end[];

int 
main()
{
    uart_init();
    puts("Initialize uart0\n");
    buddy_init((uint64) end, KERNELTOP);
    puts("Initialize buddy system\n");
    test_buddy();
    
    return 0;
}
