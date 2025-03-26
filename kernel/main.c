#include <common.h>
#include <platform.h>
#include <trap.h>
#include <mm/memlayout.h>
#include <mm/mm.h>
#include <debug.h>
#include <irq/interrupt.h>
#include <context.h>
#include <proc/proc.h>
#include <proc/sched.h>

#ifdef ARCH_RISCV
#include <uart.h>
#include <riscv.h>
#endif

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
    proc_init();
    out("Initialize first proc");

    // ecall();

    out("call scheduler");
    scheduler();

    out("main return successfully");
    return 0;
}
