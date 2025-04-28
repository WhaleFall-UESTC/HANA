#include <common.h>
#include <mm/memlayout.h>
#include <mm/mm.h>
#include <klib.h>
#include <debug.h>
#include <loongarch.h>

// extern char trampoline[];

pagetable_t kernel_pagetable;

void
kinit()
{
    kmem_init(RAMBASE, RAMTOP);
}

void 
tlbinit()
{
    csr_write(CSR_STLBPS, PGSHIFT);
    csr_write(CSR_TLBREHI, PGSHIFT);
    csr_write(CSR_ASID, 0);
    invtlb();
}

void 
kvminit()
{
    kernel_pagetable = kvmmake();

    // pagetable config
    csr_write(CSR_PWCL, (PTBASE | (DIRWIDTH << 5) | (DIRBASE(1) << 10) | (DIRWIDTH << 15) | (DIRBASE(2) << 20) | (DIRWIDTH << 25)) | CSR_PWCL_PTEWidth64);
    csr_write(CSR_PWCH, CSR_PWCH_HPTW_En);
    csr_write(CSR_RVACFG, 8);
}

// set pagetable and enable paging
void
kvminithart()
{
    csr_write(CSR_PGDH, (uint64) kernel_pagetable);
    tlbinit();
}

pagetable_t 
kvmmake()
{
    pagetable_t kpgtbl = alloc_pagetable();

    // anything else is mapped by DMW0

    // mappages(kpgtbl, TRAMPOLINE, (uint64) trampoline, PGSIZE, PTE_PLV0 | PTE_MAT_CC | PTE_G | PTE_P | PTE_RPLV);

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
mappages(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 flags)
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

