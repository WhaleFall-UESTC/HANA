#include <common.h>
#include <mm/page.h>
#include <mm/buddy.h>
#include <mm/slab.h>
#include <debug.h>
#include <klib.h>

void*
kalloc(uint64 sz)
{
    assert(sz > 0);
    // void* res;
    // if (sz > NR_OBJS * OBJECT_SIZE)
    //     res = buddy_alloc(sz);
    // else
    //     res = slab_alloc(sz);
    // debug("kalloc: 0x%p, sz = %lu", res, sz);
    // return res;
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
    // debug("kfree: 0x%p", addr);
    if (IS_PGALIGNED(addr)) {
        buddy_free(addr, GET_PAGE_ORDER(addr, pages));
    } else {
        struct slab* s = SLAB(addr);
        int idx = OBJECT_IDX(addr);
        slab_free(addr, s->objs[idx].size);
    }
}
