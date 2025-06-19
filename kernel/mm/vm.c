#include <common.h>
#include <arch.h>
#include <mm/mm.h>
#include <proc/proc.h>
#include <debug.h>


// for fork() copy child process userspace
// add load page fault handler later 
void
uvmcopy(pagetable_t cpgtbl, pagetable_t ppgtbl, uint64 sz)
{
    // copy .text & .data & stack & heap
    for (uint64 va = 0; va < sz; va += PGSIZE) {
        pte_t *ppte = walk(ppgtbl, va, WALK_NOALLOC);
        Assert(*ppte && (*ppte & PTE_V), "Invalid pte: %lx", *ppte);
        
        uint64 pa = PTE2PA(*ppte);
        uint64 flags = PTE_FLAGS(*ppte);
        flags = ((*ppte & PTE_W) ? ((flags & ~PTE_W) | PTE_COW) : flags);

        mappages(cpgtbl, va, pa, PGSIZE, flags);
    }
}


// given userspace destination virtual address
// copy len bytes from kernel to user
int
copyout(pagetable_t pgtbl, uint64 dstva, void* src, int len)
{
    // char* s = (char*) src;
    uint64 va0 = 0, pa0 = 0;
    pte_t *pte = NULL;

    while (len > 0) {
        va0 = PGROUNDDOWN(dstva);
        
        pte = walk(pgtbl, va0, WALK_NOALLOC);
        EXIT_IF(!CHECK_PTE(pte, PTE_V | PTE_U), "copyout occurs pte illegal");
            
        pa0 = PTE2PA(pa0);
        if (pa0 == 0)
            return -1;

        if (*pte & PTE_COW) {
            char* mem = kalloc(PGSIZE);
            EXIT_IF(mem == NULL, "out of memory");

            memmove(mem, (void*) pa0, PGSIZE);

            uint64 flags = PTE_FLAGS(*pte);
            flags = (flags & ~PTE_COW) | PTE_W;

            *pte = (PA2PTE(mem) | flags);
                
            // page ref count
            // kfree((void*) pa0);

            pa0 = (uint64) mem;
        }
        
        uint64 n = PGSIZE - (dstva - va0);
        n = (n > len ? len : n);

        memmove((void*)(pa0 + dstva - va0), src, n);

        len -= n;
        src += n;
        dstva = va0 + PGSIZE;
    }

    return 0;
}