#include <common.h>
#include <debug.h>
#include <klib.h>
#include <platform.h>
#include <trap/trap.h>
#include <mm/memlayout.h>
#include <mm/mm.h>
#include <irq/interrupt.h>
#include <trap/context.h>
#include <proc/proc.h>
#include <proc/sched.h>
#include <io/virtio.h>
#include <io/blk.h>
#include <drivers/uart.h>

// #include <drivers/virtio.h>

#ifdef ARCH_RISCV
#include <riscv.h>
#elif defined(ARCH_LOONGARCH)
#include <loongarch.h>
#endif

#include <testdefs.h>

char init_stack[KSTACK_SIZE * NCPU] __attribute__((aligned(PGSIZE)));
char *init_stack_top = &init_stack[KSTACK_SIZE];

struct cpu cpus[NCPU];

int 
main()
{
    uart_init();
    out("Initialize uart0");
    kinit();
    kvminit();
    out("Initialize vm");
    kvminithart();
    out("Enable paging");
    trap_init();
    trap_init_hart();
    out("Initialize trap");
    irq_init();
    out("Initialize interrupt");
    proc_init();
    out("Initialize first proc");
    
    // ecall();
    
    blocks_init();
    out("Initialize block subsystem");
    
    virtio_init();
    out("Initialize virtio");

    test_proc_init((uint64) test);

    // test_virtio();

    out("call scheduler");
    scheduler();

    out("main return successfully");
    return 0;
}
