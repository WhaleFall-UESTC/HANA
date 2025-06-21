#ifndef __MM_H__
#define __MM_H__

#include <klib.h>
#include <arch.h>

extern pagetable_t kernel_pagetable;

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
int         copyout(pagetable_t pgtbl, uint64 dstva, void* src, int len);
uint64      virt_to_phys(uint64 va);
uint64      phys_to_virt(uint64 pa);
void        uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free);

void        store_page_fault_handler();
void        page_unmap_handler();

static inline uint64 phys_page_number(uint64 pa) {
    return pa >> PGSHIFT;
}

#include <mm/page.h>

#define IS_DATA(addr) (((addr) >= (uint64) pages) && ((addr) < PHYSTOP)) 

#endif // __MM_H__