#include <common.h>
#include <mm/buddy.h>
#include <mm/slab.h>
#include <debug.h>
#include <klib.h>

struct page* pages;

void
kmem_init(uint64 va_start, uint64 va_end)
{
    uint64 s = PGROUNDUP(va_start);
    uint64 e = PGROUNDDOWN(va_end);
    debug("RAM starts at %lx, end at %lx", s, e);
    uint64 npages = ((e - s) >> PGSHIFT);
    pages = (struct page*) s;
    s += npages * sizeof(struct page); // should pgroundup, but has bug here
    debug("Initialize npages %#lx, a single page takes %d bytes", npages, (int)sizeof(struct page));
    buddy_init(s, e);  
    out("Initialize buddy system");
    slab_init();
    out("Initialize slab");
}


void*
kalloc(uint64 sz)
{
    // log("kalloc sz: %lu", sz);
    assert(sz > 0);
    if (sz > NR_OBJS * OBJECT_SIZE)
        return buddy_alloc(sz);
    else
        return slab_alloc(sz);
}

void *
kcalloc(uint64 nr, uint64 sz)
{
    void* addr = kalloc(nr*sz);
    memset(addr, 0, nr*sz);
    return addr;
}

void
kfree(void *addr)
{
    // log("kfree addr: %lx", (uint64)addr);
    if (IS_PGALIGNED(addr)) {
        buddy_free(addr, GET_PAGE_ORDER(addr, pages));
    } else {
        struct slab* s = SLAB(addr);
        int idx = OBJECT_IDX(addr);
        slab_free(addr, s->objs[idx].size);
    }
}
