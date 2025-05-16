#include <common.h>
#include <klib.h>
#include <loongarch.h>
#include <debug.h>
#include <mm/mm.h>
#include <mm/memlayout.h>
#include <trap/trap.h>
#include <trap/context.h>
#include <irq/interrupt.h>
#include <proc/proc.h>
#include <proc/sched.h>

typedef int (*putchar_t)(int);
extern putchar_t put_char;

extern void timer_enable();

// temporary stack for boot
char init_stack[KSTACK_SIZE * NCPU] __attribute__((aligned(PGSIZE)));
struct cpu cpus[NCPU];

void uart_init(void);

void __info_exception() {
    Log(ANSI_FG_RED, "ERA: %lx", r_csr_era());
    uint64 estat = r_csr_estat();
    int ecode = ((estat & CSR_ESTAT_Ecode) >> 16);
    int esubcode = ((estat & CSR_ESTAT_EsubCode) >> 22);
    Log(ANSI_FG_RED, "Ecode: %d, Esubcode: %d", ecode, esubcode);
    Log(ANSI_FG_RED, "BADV: %lx", r_csr_badv());
}

__attribute__((aligned(PGSIZE))) void __panic_exception() {
    __info_exception();
    panic("Exception");
}

void test_kvm() {
    w_csr_eentry((uint64)__panic_exception);
    uint64* tmp = kalloc(PGSIZE);
    debug("tmp addr: %p", tmp);
    uint64 pa = KERNEL_VA2PA(tmp);
    log("map va %lx to pa %lx", KSTACK(0), pa);
    mappages(kernel_pagetable, KSTACK(0), pa, PGSIZE, PTE_P | PTE_NX | PTE_PLV0 | PTE_RPLV | PTE_W | PTE_MAT_CC | PTE_D);
    *((uint64*)KSTACK(0)) = 0x114514;
    *tmp = 0x01919810UL;
    log("va read: %lx", *((uint64*)KSTACK(0)));
    assert(*((uint64*)KSTACK(0)) == 0x01919810UL);
    PASS("pass kvm test");
}

int main() {
    uart_init();
    debug("CRMD: %lx", r_csr_crmd());
    debug("DMW0: %lx", r_csr_dmw0());
    PASS("loongarch64 start!!!");

    kinit();
    log("kinit");
    kvminit();
    log("kvminit");
    kvminithart();
    test_kvm();

    trap_init();
    trap_init_hart();
    log("trap init");
    
    proc_init();

    intr_on();
    timer_enable();

    debug("tcfg: %lx ecfg: %lx crmd:%lx", r_csr_tcfg(), r_csr_ecfg(), r_csr_crmd());
    
    scheduler();
}