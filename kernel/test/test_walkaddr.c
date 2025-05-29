#include <common.h>
#include <klib.h>
#include <debug.h>
#include <arch.h>

#include <mm/mm.h>
#include <mm/memlayout.h>

extern pagetable_t kernel_pagetable;

void test_walkaddr() {
    uint64 start_va = KERNELBASE;
    uint64 end_va = PHYSTOP;

    for (uint64 addr = start_va; addr < end_va; addr+= PGSIZE) {
        uint64 pa1 = walkaddr(kernel_pagetable, addr);
        Assert(pa1 == addr, "pa1: %lx, va: %lx", pa1, addr);
        uint64 pa2 = walkaddr(kernel_pagetable, addr + 1);
        Assert(pa2 == addr + 1, "pa2: %lx, va: %lx", pa2, addr + 1);
    }

    PASS("test_walkaddr");
}