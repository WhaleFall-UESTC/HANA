#include <common.h>
#include <arch.h>
#include <mm/mm.h>
#include <mm/memlayout.h>
#include <proc/proc.h>
#include <irq/interrupt.h>

#include <syscall.h>


uint64 
find_free_vma_range(struct proc *p, size_t length) 
{
    uint64 hint = p->mmap_brk - length; 
    struct vm_area *vma;
    
    for (vma = p->vma_list; vma; vma = vma->next) {
        if (hint + length <= vma->start) {
            return hint;
        }
        hint = vma->end - length; 
    }
    
    if (hint < p->mmap_base) {
        uint64 expand_size = MAX(length, MMAP_EXPAND);
        uint64 new_brk = p->mmap_brk + expand_size;

        // TODO: CHECK BOUND
        
        p->mmap_brk = new_brk;
        return 0; 
    }
    return hint;
}

struct vm_area* 
find_vma(struct proc* p, uint64 va)
{
    struct vm_area* vma;
    for (vma = p->vma_list; vma; vma = vma->next) {
        if (va >= vma->start && va < vma->end)
            return vma;
    }
    return NULL;
}

// suppose cond is true, if not, return MMAP_FAILED
#define MMAP_CHECK(cond) \
    if (!(cond)) return MMAP_FAILED


SYSCALL_DEFINE6(mmap, void*, void*, addr, size_t, length, int, prot, int, flags, int, fd, off_t, offset) 
{
    struct proc* p = myproc();

    MMAP_CHECK(length != 0 && IS_PGALIGNED(offset));

    length = PGROUNDUP(length);

    uint64 va;
    if (flags & MAP_FIXED) {
        va = (uint64) addr;
        MMAP_CHECK(IS_PGALIGNED(va));
    }
    else {
        va = find_free_vma_range(p, length);
        MMAP_CHECK(va);
    }

    KALLOC(struct vm_area, vma);
    vma->start = va;
    vma->end = va + length;
    vma->prot = prot;
    vma->flags = flags;
    vma->offset = offset;

    if (flags & MAP_ANONYMOUS) {
        vma->file = NULL;
    } else {
        // filesystem, help to check fd valid
        // MMAP_CHECK(fd >= 0 && fd < NR_OPEN && p->fdt[fd]);
        // vma->file = filedup(p->fdt[fd]);
    }

    vma->next = p->vma_list;
    if (p->vma_list) p->vma_list->prev = vma;
    p->vma_list = vma;
    
    return NULL;
}

SYSCALL_DEFINE2(munmap, int, void*, addr, size_t, length)
{
    uint64 va = PGROUNDDOWN(addr);
    length = PGROUNDUP(length);
    if (length == 0) return -1;

    struct proc* p = myproc();

    struct vm_area* vma = find_vma(p, va);
    if (!vma) return -1;

    uint64 unmap_start = MAX(va, vma->start);
    uint64 unmap_end = MIN(va + length, vma->end);
    size_t unmap_len = unmap_end - unmap_start;

    uvmunmap(UPGTBL(p->pagetable), va, (unmap_len >> PGSHIFT), UVMUNMAP_FREE);
    
    if (unmap_start == vma->start && unmap_end == vma->end) {
        if (vma->prev) {
            vma->prev->next = vma->next;
            if (vma->next) vma->next->prev = vma->prev;
        } else {
            p->vma_list = vma->next;
            vma->next->prev = NULL;
        } 

        // if (vma->file) closefile
        kfree(vma);
    }
    else if (unmap_start == vma->start) {
        vma->start = unmap_end;
    }
    else if (unmap_end == vma->end) {
        vma->end = unmap_start;
    }
    else {
        KALLOC(struct vm_area, new_vma);
        memmove(new_vma, vma, sizeof(struct vm_area));
        new_vma->start = unmap_end;
        vma->end = unmap_start;
        new_vma->next = vma->next;
        vma->next = new_vma;
        if (new_vma->next) new_vma->next->prev = new_vma;
        new_vma->prev = vma;
    }

    return 0;
}

SYSCALL_DEFINE1(brk, uintptr_t, uintptr_t, brk)
{

    return 0;
}


