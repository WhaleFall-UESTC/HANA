#include <riscv.h>
#include <platform.h>
#include <memlayout.h>
#include <defs.h>
#include <debug.h>

#include <testdefs.h>

char init_stack[PGSIZE] __attribute__((aligned(PGSIZE)));
char *init_stack_top = &init_stack[PGSIZE];

extern char end[];

int 
main()
{
    uart_init();
    out("Initialize uart0");
    buddy_init((uint64) end, KERNELTOP);
    out("Initialize buddy system");
    slab_init();
    
    return 0;
}
