#include <common.h>
#include <mm/memlayout.h>
#include <mm/mm.h>
#include <klib.h>
#include <platform.h>
#include <debug.h>

#ifdef ARCH_RISCV
#include <riscv.h>
#endif


extern char etext[];
extern char trampoline[];
extern char *init_stack_top;

pagetable_t kernel_pagetable;

static inline pagetable_t
alloc_pagetable()
{
    pagetable_t pgtbl = (pagetable_t) kalloc(PGSIZE);
    memset(pgtbl, 0, PGSIZE);
    return pgtbl;
}


void 
kvminit()
{
    kernel_pagetable = kvmmake();
    // mappages(kernel_pagetable, KSTACK(0), (uint64)init_stack_top, PGSIZE, PTE_R | PTE_W);
}

// set pagetable and enable paging
void
kvminithart()
{
    w_satp(MAKE_SATP(kernel_pagetable));
    sfence_vma();

    // init_stack is in rodata
    // which va mapped to the same pa
}


pagetable_t
kvmmake()
{
    pagetable_t kpgtbl = alloc_pagetable();

    // uart0
    mappages(kpgtbl, UART0, UART0, VIRT_UART0_SIZE, PTE_R | PTE_W);

    // virtio
    mappages(kpgtbl, VIRTIO0, VIRTIO0, VIRT_VIRTIO_SIZE, PTE_R | PTE_W);

    // CLINT
    mappages(kpgtbl, CLINT, CLINT, VIRT_CLINT_SIZE, PTE_R | PTE_W);

    // PLIC
    mappages(kpgtbl, PLIC, PLIC, VIRT_PLIC_SIZE, PTE_R | PTE_W);

    // kernel .text
    mappages(kpgtbl, KERNELBASE, KERNELBASE, (uint64)etext - KERNELBASE, PTE_R | PTE_X);

    // kernel data and physical memory space
    mappages(kpgtbl, (uint64)etext, (uint64)etext, PHYSTOP - (uint64)etext, PTE_R | PTE_W);

    // map trampoline page
    mappages(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

    return kpgtbl;
}


// find the pte's address of given va
// when pte in L2, L1 is NULL and alloc is set
// walk will alloc pagetable
// if alloc == 0 or kalloc failed, return NULL
pte_t*
walk(pagetable_t pgtbl, uint64 va, int alloc)
{
    va >>= 12;
    for (int shift = 18; shift > 0; shift -= 9) {
        int idx = (va >> shift) & 0x1ff;
        pte_t* pte = pgtbl + idx;
        if ((*pte & PTE_V) == 0) {
            if (alloc && (*pte = PA2PTE(alloc_pagetable())) != 0)
                *pte |= PTE_V;
            else return 0;
        } 
        pgtbl = (pagetable_t) PTE2PA(*pte);
    }

    return (pgtbl + (va & 0x1ff));
}


void
mappages(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, int flags)
{
    uint64 start_va = PGROUNDDOWN(va);
    uint64 end_va = PGROUNDUP(va + sz - 1);
    int npages = (end_va - start_va) >> PGSHIFT;
    pte_t *pte = walk(pgtbl, va, WALK_ALLOC);
    assert(pte);
    int nr_mapped = 0;

    while (nr_mapped++ < npages) {
        *pte++ = PA2PTE(pa) | flags | PTE_V;
        pa += PGSIZE;
        // if this is the last pte in L0 pgtbl, start from another pgtbl
        if (IS_PGALIGNED(pte)) {
            pte = walk(pgtbl, va + nr_mapped * PGSIZE, WALK_ALLOC);
            assert(pte);
        }
    }
}


// Look up va in given pgtbl
// return its pa or NULL if not mapped
uint64
walkaddr(pagetable_t pgtbl, uint64 va)
{
    if (va >= MAXVA)
        return 0;

    pte_t* pte = walk(pgtbl, va, WALK_NOALLOC);

    // if (!CHECK_PTE(pte, PTE_V | PTE_U))
    if (!CHECK_PTE(pte, PTE_V))
        return 0;

    return (uint64) PTE2PA(*pte);
}


// make user pagetable
// user_pa: address of a 2 PGSIZE space
pagetable_t
uvmmake(uint64 trapframe)
{
    pagetable_t upgtbl = alloc_pagetable();

    // map TRAMPOLINE
    mappages(upgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

    // map TRAPFRAME
    mappages(upgtbl, TRAPFRAME, trapframe, PGSIZE, PTE_R | PTE_W);

    return upgtbl;
}


// just for init proc
pagetable_t
uvminit(uint64 trapframe, char* init_code, int sz)
{
    assert(init_code);
    assert(sz <= PGSIZE);
    
    void* userspace = kalloc(2*PGSIZE);
    memmove(userspace, init_code, sz);

    pagetable_t upgtbl = uvmmake(trapframe);

    mappages(upgtbl, 0, (uint64)userspace, PGSIZE, PTE_U | PTE_R | PTE_X);

    // map guard page, for uvmcpoy
    mappages(upgtbl, PGSIZE, 0, PGSIZE, 0);

    mappages(upgtbl, 2 * PGSIZE, (uint64)userspace + PGSIZE, PGSIZE, PTE_U | PTE_R | PTE_W);

    return upgtbl;
}


// for fork() copy child process userspace
void
uvmcopy(pagetable_t cpgtbl, pagetable_t ppgtbl, uint64 sz)
{
    // copy .text & .data & stack & heap
    for (uint64 addr = 0; addr < sz;) {
        pte_t *ppte = walk(ppgtbl, addr, WALK_NOALLOC);
        assert(ppte);
        do {
            pte_t *cpte = walk(cpgtbl, addr, WALK_ALLOC);
            assert(cpte);
            uint64 cpa = PTE2PA(*cpte);
            uint64 ppa = PTE2PA(*ppte);
            if (ppa)
                memmove((void*)cpa, (void*)ppa, PGSIZE);
            *cpte |= PTE_FLAGS(*ppte);

            addr += PGSIZE;
            ppte++;
        } while (addr < sz && !IS_PGALIGNED(ppte));
    }
}
