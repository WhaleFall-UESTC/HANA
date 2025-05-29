#include <common.h>
#include <debug.h>
#include <arch.h>
#include <mm/mm.h>
#include <mm/memlayout.h>

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
    uint64 va = TRAMPOLINE - PGSIZE;
    log("map va %lx to pa %lx", va, pa);
    mappages(kernel_pagetable, va, pa, PGSIZE, PTE_P | PTE_NX | PTE_PLV0 | PTE_RPLV | PTE_W | PTE_MAT_CC | PTE_D);
    *((uint64*)va) = 0x114514;
    *tmp = 0x01919810UL;
    log("va read: %lx", *((uint64*)va));
    assert(*((uint64*)va) == 0x01919810UL);
    PASS("pass kvm test");
}

void test_arch() {
    test_kvm();
}