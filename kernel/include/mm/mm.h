#ifndef __MM_H__
#define __MM_H__

#ifdef ARCH_RISCV
#include <riscv.h>
#endif

extern pagetable_t kernel_pagetable;

void        kinit();
void*       kalloc(uint64 sz);
void*       kcalloc(uint64 nr, uint64 sz);
void        kfree(void *addr);

void        kvminit();
void        kvminithart();
pagetable_t kvmmake();
pte_t*      walk(pagetable_t pgtbl, uint64 va, int alloc);
void        mappages(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, int flags);
uint64      walkaddr(pagetable_t pgtbl, uint64 va);
pagetable_t uvmmake(uint64 trapframe);
pagetable_t uvminit(uint64 trapframe, char* init_code, int sz);
void        uvmcopy(pagetable_t cpgtbl, pagetable_t ppgtbl, uint64 sz);

static inline uint64 virt_to_phys(uint64 va) {
    return va;
    // return walkaddr(kernel_pagetable, va);
}

static inline uint64 phys_page_number(uint64 pa) {
    return pa >> PGSHIFT;
}

#endif // __MM_H__