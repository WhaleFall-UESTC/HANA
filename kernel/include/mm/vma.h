#ifndef __VMA_H__
#define __VMA_H__

#include <common.h>
#include <fs/file.h>

struct vm_area {
    uint64 start;
    uint64 end;
    int prot;
    int flags;
    struct file *file;
    off_t offset;
    struct vm_area *next;
    struct vm_area *prev;
    int refcnt;
};

uint64 find_free_vma_range(struct proc *p, size_t length);
struct vm_area* find_vma(struct proc* p, uint64 va);

#endif