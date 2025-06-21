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

    return 0;
}

SYSCALL_DEFINE1(brk, uintptr_t, uintptr_t, brk)
{

    return 0;
}


