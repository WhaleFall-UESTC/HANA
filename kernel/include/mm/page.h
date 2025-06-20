#ifndef __PAGE_H__
#define __PAGE_H__

#include <common.h>
#include <arch.h>

#pragma pack(push, 1)

struct page {
    uint order : 4;
    uint flag : 4;
    uint8 cnt;
};

#pragma pack(pop)

extern struct page* pages;

#define GET_PAGE(addr) (pages + (((uint64)addr - (uint64)pages) >> PGSHIFT))
#define PAGES_BASE ((struct page *) end)

int page_ref_inc(uint64 pa);
int page_ref_dec(uint64 pa);

typedef struct {
    pagetable_t pgtbl;
    volatile int cnt;
} upagetable;


#define UPGTBL(upgtbl) (upgtbl->pgtbl)

pagetable_t alloc_pagetable();
upagetable* upgtbl_init(pagetable_t pagetable);
upagetable* upgtbl_clone(upagetable* upgtbl);
int upgtbl_incr(upagetable* upgtbl);
int upgtbl_decr(upagetable* upgtbl);

#endif