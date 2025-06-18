#ifndef __PAGETABLE_H__
#define __PAGETABLE_H__

#include <common.h>
#include <arch.h>

extern pagetable_t kernel_pagetable;

static inline uint64 phys_page_number(uint64 pa) {
    return pa >> PGSHIFT;
}

static inline pagetable_t alloc_pagetable() {
    pagetable_t pgtbl = (pagetable_t) kalloc(PGSIZE);
    memset(pgtbl, 0, PGSIZE);
    return pgtbl;
}

typedef struct {
    pagetable_t pgtbl;
    volatile int cnt;
} upagetable;

static inline upagetable* 
upgtbl_init(pagetable_t pagetable) {
    KALLOC(upagetable, ret);
    ret->pgtbl = pagetable;
    ret->cnt = 1;
    return ret;
}

#define UPGTBL_INCR(upgtbl) __sync_fetch_and_add(&upgtbl->cnt, 1)
#define UPGTBL_DECR(upgtbl) __sync_fetch_and_sub(&upgtbl->cnt, 1)
#define UPGTBL(upgtbl) (upgtbl->pgtbl)

static inline upagetable*
upgtbl_clone(upagetable* upgtbl) {
    UPGTBL_INCR(upgtbl);
    return upgtbl;
}

#endif