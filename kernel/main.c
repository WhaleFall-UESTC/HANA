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

// #include <drivers/virtio-mmio.h>

#include <arch.h>

#include <testdefs.h>

char init_stack[KSTACK_SIZE * NCPU] __attribute__((aligned(PGSIZE)));
char *init_stack_top = &init_stack[KSTACK_SIZE];

struct cpu cpus[NCPU];
struct page* pages;

#ifdef ARCH_LOONGARCH
extern void timer_enable();
#endif

int 
main()
{
    uart_init();
    out("Initialize uart0");

#ifdef ARCH_LOONGARCH
    debug("CRMD: %lx", r_csr_crmd());
    debug("DMW0: %lx", r_csr_dmw0());
    PASS("loongarch64 start!!!");
#endif

    kinit();
    kvminit();
    out("Initialize vm");
    kvminithart();
    out("Enable paging");

    // test_arch();

    trap_init();
    trap_init_hart();
    out("Initialize trap");
    irq_init();
    out("Initialize interrupt");
    proc_init();
    out("Initialize first proc");
    
    // ecall();

#ifdef ARCH_LOONGARCH
#include <drivers/pci.h>
    pci_init();
#endif
    udp_init();
    uart_device_init();
    virtio_device_init();

    vfilesys_init();
    out("Initialize vfs");

    out("Enter tests");
    test_proc_init((uint64) test);

#ifdef ARCH_LOONGARCH
    intr_on();
    timer_enable();
    debug("tcfg: %lx ecfg: %lx crmd:%lx", r_csr_tcfg(), r_csr_ecfg(), r_csr_crmd());
#endif 

    out("call scheduler");
    scheduler();

    out("main return successfully");
    return 0;
}
