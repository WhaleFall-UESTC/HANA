#include <platform.h>
#include <memlayout.h>
#include <defs.h>
#include <debug.h>
#include <interrupt.h>

// #include <testdefs.h>

char init_stack[KSTACK_SIZE] __attribute__((aligned(PGSIZE)));
char *init_stack_top = &init_stack[KSTACK_SIZE];

extern char end[];

int 
main()
{
    uart_init();
    out("Initialize uart0");
    kinit((uint64) end, PHYSTOP);
    kvminit();
    out("Initialize vm");
    kvminithart();
    out("Enable paging");
    trap_init();
    trap_init_hart();
    out("Initialize trap");
    interrupt_init();
    out("Initialize interrupt");

    ecall();

    for (;;) ;

    log("main return successfully");
    return 0;
}
