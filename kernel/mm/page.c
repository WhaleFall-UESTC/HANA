#include <common.h>
#include <arch.h>
#include <mm/mm.h>
#include <mm/memlayout.h>

extern char end[], etext[];

int
page_ref_inc(uint64 pa)
{
    if (pa < (uint64) pages || pa > PHYSTOP)
        return -1;
    struct page* p = GET_PAGE(pa);
    return __sync_fetch_and_add(&p->cnt, 1);
}

int
page_ref_dec(uint64 pa)
{
    struct page* p = GET_PAGE(pa);
    return __sync_fetch_and_sub(&p->cnt, 1);
}

pagetable_t alloc_pagetable() {
    pagetable_t pgtbl = (pagetable_t) kalloc(PGSIZE);
    memset(pgtbl, 0, PGSIZE);
    return pgtbl;
}

upagetable* 
upgtbl_init(pagetable_t pagetable) {
    KALLOC(upagetable, ret);
    ret->pgtbl = pagetable;
    ret->cnt = 1;
    return ret;
}

upagetable*
upgtbl_clone(upagetable* upgtbl) {
    upgtbl_incr(upgtbl);
    return upgtbl;
}

int upgtbl_incr(upagetable* upgtbl) {
    return __sync_fetch_and_add(&upgtbl->cnt, 1);
}

int upgtbl_decr(upagetable* upgtbl) {
    return __sync_fetch_and_sub(&upgtbl->cnt, 1);
}
