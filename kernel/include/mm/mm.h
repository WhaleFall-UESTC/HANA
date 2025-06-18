#ifndef __MM_H__
#define __MM_H__

#include <klib.h>
#include <arch.h>

void        kmem_init(uint64 va_start, uint64 va_end);
void        kinit();
void*       kalloc(uint64 sz);
void*       kcalloc(uint64 nr, uint64 sz);
void        kfree(void *addr);

void        kvminit();
void        kvminithart();
pagetable_t kvmmake();
pte_t*      walk(pagetable_t pgtbl, uint64 va, int alloc);
void        mappages(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 flags);
uint64      walkaddr(pagetable_t pgtbl, uint64 va);
pagetable_t uvmmake(uint64 trapframe);
pagetable_t uvminit(uint64 trapframe, char* init_code, int sz);
void        uvmcopy(pagetable_t cpgtbl, pagetable_t ppgtbl, uint64 sz);
void        map_stack(pagetable_t pgtbl, uint64 stack_va);
uint64      virt_to_phys(uint64 va);
uint64      phys_to_virt(uint64 pa);


#include <mm/pagetable.h>

#endif // __MM_H__