#ifndef __MM_H__
#define __MM_H__

#include <common.h>
#include <defs.h>

extern pagetable_t kernel_pagetable;

void kinit();
void* kalloc(uint64 sz);
void kfree(void *addr);

static inline uint64 virt_to_phys(uint64 va) {
    return walkaddr(kernel_pagetable, va);
}

#endif // __MM_H__