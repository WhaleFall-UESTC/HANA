#include <defs.h>
#include <slab.h>
#include <buddy.h>
#include <debug.h>

struct slab* current;
struct slab* partial;
uint partial_len = 0;

static inline void
set_object_entry(struct object_entry* entry, uint8 size, int8 prev, int8 next)
{
    entry->size = size;
    entry->prev = prev;
    entry->next = next;
}

static inline void
init_slab(void* addr)
{
    struct slab* ret = (struct slab*) addr;
    ret->next = 0;
    set_object_entry(&ret->sentinel, NR_OBJS, 0, 0);
    set_object_entry(ret->objs, NR_OBJS, OBJECT_SENTINEL, OBJECT_SENTINEL);
    memset(&ret->objs[1], 0, (NR_OBJS - 2) * sizeof(struct object_entry));
    set_object_entry(&ret->objs[NR_OBJS - 1], NR_OBJS, E, D);
}

void 
slab_init()
{
    assert(sizeof(struct slab) == PGSIZE);
    struct slab* partial = buddy_alloc(4 * PGSIZE);
    for (int i = 0; i < 4; i++)
        init_slab(partial + i);
    SLAB_SET_NEXT(partial, partial + 1);
    SLAB_SET_NEXT(partial + 1, partial + 2);
    current = partial + 3;
    partial_len = 3;
}


static inline void*
alloc_objs(struct slab* slab, uint8 nr_objs)
{
    if (NR_FREE_OBJS(slab) < nr_objs)
        return NULL;

    for (int idx = 0; idx != OBJECT_SENTINEL; idx = slab->objs[idx].next) {
        if (slab->objs[idx].size >= nr_objs) {
            // set old end zero
            set_object_entry(&slab->objs[idx - 1 + slab->objs[idx].size], 0, 0, 0);
            // reduce the size
            slab->objs[idx].size -= nr_objs;
            slab->sentinel.size -= nr_objs;
            // set new end
            struct object_entry* end = &slab->objs[idx - 1 + slab->objs[idx].size];
            set_object_entry(end, slab->objs[idx].size, E, D);
            // return result
            return (void*)(&end[1]);
        }
    }

    return NULL;
}


void*
slab_alloc(uint64 sz)
{
    assert(sz <= (NR_OBJS * OBJECT_SIZE));
    uint8 nr_objs = ROUNDUP(sz, OBJECT_SIZE);
    void* current_alloc = alloc_objs(current, nr_objs);
    if (current_alloc)
        return current_alloc;

    struct slab *ptr_pre = partial, *ptr = SLAB_NEXT(partial);
    // if the head of partial can alloc objs
    if (ptr_pre->sentinel.size >= nr_objs) {
        // swap current and partial head
        SLAB_SET_NEXT(current, ptr);
        SLAB_SET_NEXT(partial, 0);
        struct slab* tmp = current;
        current = partial;
        partial = tmp;
        // return alloc result
        return alloc_objs(current, nr_objs);
    }

    // find valid slab in partial
    while (ptr->next != 0) {
        if (ptr->sentinel.size >= nr_objs) {
            struct slab* ptr_next = SLAB_NEXT(ptr);
            SLAB_SET_NEXT(current, ptr_next);
            SLAB_SET_NEXT(ptr_pre, current);
            current = ptr;
            return alloc_objs(current, nr_objs);
        }
        ptr_pre = ptr;
        ptr = SLAB_NEXT(ptr);
    }

    // if there is none, ask for buddy system
    // add current to the partial list
    SLAB_SET_NEXT(current, partial);
    partial = current;
    partial_len++;
    current = (struct slab*) buddy_alloc(PGSIZE);
    init_slab(current);
    return alloc_objs(current, nr_objs);
}



static inline void 
merge_high(struct slab* slab, int idx, uint8 nr_free)
{
    struct object_entry below = slab->objs[idx + nr_free];
    // set objs[idx]
    set_object_entry(&slab->objs[idx], nr_free + below.size, below.prev, below.next);
    // set prev object_entry and reset its next
    slab->objs[(int)below.prev].next = idx;
    // reset end
    slab->objs[idx + nr_free + below.size - 1].size += nr_free;
    // clean below object_entry
    set_object_entry(&slab->objs[idx + nr_free], 0, 0, 0);
}

static inline void 
merge_low(struct slab* slab, int idx, uint8 nr_free)
{
    struct object_entry* upper = &slab->objs[idx - slab->objs[idx - 1].size];
    upper->size += nr_free;
    set_object_entry(&slab->objs[idx - 1], 0, 0, 0);
    set_object_entry(&slab->objs[idx - 1 + (int)nr_free], upper->size, E, D);
}


void
slab_free(void* addr, uint8 nr_free)
{
    struct slab* slab = SLAB(addr);
    slab->sentinel.size += nr_free;
    if (slab->sentinel.size == NR_OBJS && partial_len > MIN_PARTIAL) {
        buddy_free(addr, 0);
        partial_len--;
        return;
    }
    int idx = OBJECT_IDX(addr);
    bool merge_h = false, merge_l = false;

    if (slab->objs[idx + nr_free].size != 0) {
        merge_high(slab, idx, nr_free);
        merge_h = true;
    }

    if (slab->objs[idx - 1].prev == E && slab->objs[idx - 1].next == D) {
        merge_low(slab, idx, nr_free);
        merge_l = true;
    }

    if (!merge_h && !merge_l) {
        int8 last = slab->sentinel.prev;
        slab->objs[(int)last].next = idx;
        slab->sentinel.prev = idx;
        set_object_entry(&slab->objs[idx], nr_free, last, OBJECT_SENTINEL);
    }
}
