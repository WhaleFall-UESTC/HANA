#include <common.h>
#include <arch.h>
#include <mm/buddy.h>
#include <mm/slab.h>
#include <mm/mm.h>
#include <mm/memlayout.h>
#include <proc/proc.h>
#include <debug.h>

void
kmem_init(uint64 va_start, uint64 va_end)
{
    uint64 s = PGROUNDUP(va_start);
    uint64 e = PGROUNDDOWN(va_end);
    // debug("RAM starts at %lx, end at %lx", s, e);
    uint64 npages = ((e - s) >> PGSHIFT);
    pages = (struct page*) s;
    uint64 npages_size = PGROUNDUP(npages * sizeof(struct page));
    memset((void*) s, 0, npages_size);
    s += npages_size; // should pgroundup, but has bug here
    // debug("Initialize npages %#lx, a single page takes %d bytes", npages, (int)sizeof(struct page));
    buddy_init(s, e);  
    out("Initialize buddy system");
    slab_init();
    out("Initialize slab");
}


void
mappages(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 flags)
{
    uint64 start_va = PGROUNDDOWN(va);
    uint64 end_va = PGROUNDUP(va + sz - 1);
    int npages = (end_va - start_va) >> PGSHIFT;
    pte_t* pte = walk(pgtbl, va, WALK_ALLOC);
    assert(pte);
    int nr_mapped = 0;

    while (nr_mapped++ < npages) {
        *pte = PA2PTE(pa) | flags | PTE_V;
        pte++;
        page_ref_inc(pa);
        pa += PGSIZE;
        // if this is the last pte in L0 pgtbl, start from another pgtbl
        if (IS_PGALIGNED(pte)) {
            pte = walk(pgtbl, va + nr_mapped * PGSIZE, WALK_ALLOC);
            assert(pte);
        }
    }
}


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
// return 0 on success
int
copyout(pagetable_t pgtbl, uint64 dstva, void* src, size_t len)
{
    // char* s = (char*) src;
    uint64 va0 = 0, pa0 = 0;
    pte_t *pte = NULL;

    while (len > 0) {
        va0 = PGROUNDDOWN(dstva);
        
        pte = walk(pgtbl, va0, WALK_NOALLOC);
        EXIT_IF(!CHECK_PTE(pte, PTE_V | PTE_U), "copyout occurs pte illegal");
            
        pa0 = KERNEL_PA2VA(PTE2PA(pa0));
        if (pa0 == 0)
            return -1;

        if (*pte & PTE_COW) {
            char* mem = kalloc(PGSIZE);
            EXIT_IF(mem == NULL, "out of memory");

            memmove(mem, (void*) pa0, PGSIZE);

            uint64 flags = PTE_FLAGS(*pte);
            flags = (flags & ~PTE_COW) | PTE_W;

            *pte = (PA2PTE(mem) | flags);
                
            if (page_ref_dec(pa0) == 1)  // ref count is zero now
                kfree((void*) pa0);

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


// copy from user to kernel
// if dst is NULL will alloc a space for it
int 
copyin(pagetable_t pagetable, char* dst, uint64 srcva, size_t len)
{
    uint64 va0 = 0, pa0 = 0;

    if (dst == NULL) dst = kalloc(len);
    if (dst == NULL) return -1;

    while (len > 0) {
        va0 = PGROUNDDOWN(srcva);
        pa0 = walkaddr(pagetable, va0);
        if (pa0 == 0) {
            Log(ANSI_FG_RED, "va0 %lx not found", va0);
            return -1;
        }

        uint64 offset = srcva - va0;
        uint64 n = PGSIZE - offset;
        n = (n > len ? len : n);

        memmove(dst, (void*)(KERNEL_PA2VA(pa0 + offset)), n);

        len -= n;
        dst += n;
        srcva = va0 + PGSIZE;
    }

    return 0;
}

size_t
copyinstr(pagetable_t pagetable, char* dst, uint64 srcva, size_t max)
{
    int got_null = 0;
    pagetable = (pagetable_t) KERNEL_PA2VA(pagetable);
    size_t cnt = 0;

    while (got_null == 0 && max > 0) {
        uint64 va0 = PGROUNDDOWN(srcva);
        uint64 pa0 = walkaddr(pagetable, va0);
        if (pa0 == 0) return -1;

        uint64 n = PGSIZE - (srcva - va0);
        n = (n > max ? max : n);

        char *p = (char *) KERNEL_PA2VA(pa0 + (srcva - va0));
        while (n > 0) {
            cnt++;
            if (*p == '\0') {
                *dst = '\0';
                got_null = 1;
                break;
            } else {
                *dst = *p;
            }

            --n;
            --max;
            p++;
            dst++;
        }
    }

    return (got_null ? cnt : -1);
}


ssize_t copy_from_user(void *to, const void *from, size_t n) {
    if(copyin(UPGTBL(myproc()->pagetable), (char *) to, (uint64) from, n) < 0)
        return -1;
    return n;
}

ssize_t copy_to_user(void *to, const void *from, size_t n) {
    if(copyout(UPGTBL(myproc()->pagetable), (uint64) to, (void *) from, n) < 0)
        return -1;
    return n;
}

ssize_t copy_from_user_str(char* to, const void* from, size_t max) {
    return copyinstr(UPGTBL(myproc()->pagetable), to, (uint64)from, max);
}

ssize_t copy_to_user_str(void* to, const char* from, size_t max) {
    size_t len = min_uint64(strlen(from) + 1, max);
    if(copy_to_user(to, (const void*)from, len) < 0)
        return -1;
    return len;
}

void
store_page_fault_handler()
{
    log("store fault");
    // struct proc* p = myproc();
    uint64 badv = trap_get_badv();
    uint64 va = PGROUNDDOWN(badv);

    // pte_t *pte = walk(UPGTBL(p->pagetable), va, WALK_NOALLOC);
    pte_t *pte = walk(kernel_pagetable, va, WALK_NOALLOC);
    EXIT_IF(pte == NULL, "addr %lx pte not found", badv);

    // if (!CHECK_PTE(pte, PTE_V | PTE_U | PTE_COW)) {
    if ((*pte == 0) || ((*pte & PTE_V) == 0) || ((*pte & PTE_COW) == 0)) {
        Log(ANSI_FG_RED, "*pte: %lx\tV: %lx\tU: %lx\tCOW: %lx", *pte, (*pte & PTE_V), (*pte & PTE_U), (*pte & PTE_COW));
        kernel_trap_error();
    }

    char* mem = kalloc(PGSIZE);
    uint64 pa = KERNEL_PA2VA(PTE2PA(*pte));
    memmove(mem, (void*) pa, PGSIZE);

    uint64 flags = PTE_FLAGS(*pte);
    flags = ((flags & ~PTE_COW) | PTE_W);
    // mappages(kernel_pagetable, va, KERNEL_VA2PA(mem), PGSIZE, flags);
    *pte = (PA2PTE(mem) | flags);
    page_ref_inc((uint64) mem);

    if (page_ref_dec(pa) == 1)
        kfree((void*) pa);

#ifdef ARCH_LOONGARCH
    flush_tlb_one(myproc()->pid, va);
#endif

    log("store page fault handle");
}

void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
    assert(IS_PGALIGNED(va));
    pagetable = (pagetable_t) KERNEL_PA2VA(pagetable);

    for (uint64 addr = va; addr < va + (npages << PGSHIFT); addr += PGSIZE) {
        pte_t* pte = walk(pagetable, addr, WALK_NOALLOC);
        assert(pte && (*pte & PTE_V));
        if (do_free) {
            uint64 pa = KERNEL_PA2VA(PTE2PA(*pte));
            if (page_ref_dec(pa) == 1)
                kfree((void*) pa);
        }
        *pte = 0;
#ifdef ARCH_LOONGARCH
        flush_tlb_one(myproc()->pid, addr);
#endif
    }
}


// free pagetable itself
void freewalk(pagetable_t pgtbl, int level) {
    if (level > 2)
        return;

    pgtbl = (pagetable_t) KERNEL_PA2VA(pgtbl);
    int npte = PGSIZE / sizeof(pte_t);

    for (int i = 0; i < npte; i++) {
        pte_t pte = pgtbl[i];
        uint64 child = PTE2PA(pte);
        freewalk((pagetable_t) child, level + 1);
        pgtbl[i] = 0;
    }

    kfree(pgtbl);
}


// free all physical space in pagetable
void uvmfree(pagetable_t pgtbl, uint64 sz) 
{   
    pgtbl = (pagetable_t) KERNEL_PA2VA(pgtbl);
    if (sz > 0)
        uvmunmap(pgtbl, 0, (PGROUNDUP(sz) >> PGSHIFT), UVMUNMAP_FREE);
    freewalk(pgtbl, 0);
}   
