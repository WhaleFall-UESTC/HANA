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
#include <init.h>
#include <io/blk.h>
#include <io/chr.h>
#include <fs/fs.h>
#include <drivers/uart.h>
#include <net/net.h>
#include <fs/kernel.h>

#include <arch.h>

#include <testdefs.h>

// 每一个核的初始栈
char init_stack[KSTACK_SIZE * NCPU] __attribute__((aligned(PGSIZE)));
char *init_stack_top = &init_stack[KSTACK_SIZE];

// cpu 结构体数组，根据 cpuid 访问对应的结构体
struct cpu cpus[NCPU];
// 管理内核 end 到 RAMTOP 所有物理页
struct page* pages;

#ifdef __loongarch64
extern void timer_enable();
#endif

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

#ifdef __loongarch64
#include <drivers/pci.h>
    pci_init();
#endif
    udp_init();
    uart_device_init();
    virtio_device_init();
    out("initialize io devices");

    vfilesys_init();
    out("Initialize vfs");

    out("call scheduler");
    scheduler();

    error("main: should not reach here");
    return 0;
}
