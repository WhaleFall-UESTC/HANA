#include <common.h>
#include <klib.h>
#include <loongarch.h>
#include <debug.h>
#include <mm/mm.h>
#include <mm/memlayout.h>

typedef int (*putchar_t)(int);
extern putchar_t put_char;

// temporary stack for boot
char init_stack[KSTACK_SIZE * NCPU] __attribute__((aligned(PGSIZE)));

void uart_init(void);

void test_kvm() {
    uint64* tmp = kalloc(PGSIZE);
    debug("tmp addr: %p", tmp);
    mappages(kernel_pagetable, TRAPFRAME, (((uint64)tmp) | DMW_MASK), PGSIZE, PTE_P | PTE_NX | PTE_PLV0 | PTE_RPLV);
    *tmp = 0x01919810UL;
    assert(*((uint64*)TRAPFRAME) == 0x114514);
    PASS("pass kvm test");
}

int main() {
    uart_init();
    debug("CRMD: %lx", r_csr_crmd());
    debug("DMW0: %lx", r_csr_dmw0());
    PASS("loongarch64 start!!!");
    kinit();
    kvminit();
    kvminithart();
    test_kvm();
}