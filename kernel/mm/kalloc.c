#include <defs.h>
#include <buddy.h>
#include <slab.h>
#include <debug.h>

struct page* pages;

void
kinit(uint64 start, uint64 end)
{
    uint64 s = PGROUNDUP(start);
    uint64 e = PGROUNDDOWN(end);
    uint npages = ((e - s) >> PGSHIFT);
    pages = (struct page*) s;
    s += PGROUNDUP(npages * sizeof(struct page));
    log("Initialize pages take %#x size, a single page takes %d bytes", (uint)PGROUNDUP(npages * sizeof(struct page)), (int)sizeof(struct page));
    buddy_init(s, e);  
    out("Initialize buddy system");
    slab_init();
    out("Initialize slab");
}

void*
kalloc(uint64 sz)
{
    assert(sz > 0);
    if (sz > NR_OBJS * OBJECT_SIZE)
        return buddy_alloc(sz);
    else
        return slab_alloc(sz);
}

void
kfree(void *addr)
{
    if (IS_PGALIGNED(addr)) {
        buddy_free(addr, GET_PAGE_ORDER(addr, pages));
    } else {
        struct slab* s = SLAB(addr);
        int idx = OBJECT_IDX(addr);
        slab_free(addr, s->objs[idx].size);
    }
}
