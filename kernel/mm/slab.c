#include <defs.h>
#include <slab.h>
#include <buddy.h>
#include <debug.h>

struct slab* current;
struct slab* partial;
uint partial_len = 0;

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
    partial = buddy_alloc(4 * PGSIZE);
    for (int i = 0; i < 4; i++)
        init_slab(partial + i);
    set_slab_next(partial, partial + 1);
    set_slab_next(partial + 1, partial + 2);
    current = partial + 3;
    partial_len = 3;
}


static void*
alloc_objs(struct slab* slab, uint8 nr_objs)
{
    if (nr_free_objs(slab) < nr_objs)
        return NULL;

    for (int idx = slab->sentinel.next; idx != OBJECT_SENTINEL; idx = slab->objs[idx].next) {
        if (slab->objs[idx].size >= nr_objs) {
            // set old end zero
            set_object_entry(&slab->objs[idx - 1 + slab->objs[idx].size], 0, 0, 0);
            // reduce the size
            slab->objs[idx].size -= nr_objs;
            slab->sentinel.size -= nr_objs;

            if (slab->objs[idx].size == 0) {
                // just remove these objs from list
                list_remove(slab, idx);
                set_object_entry(&slab->objs[idx], 0, 0, 0);
                return (void*) &slab->objects[idx];
            }
            else {
                // set new end
                int end_idx = idx - 1 + slab->objs[idx].size;
                // while, I forget the case that size only left 1
                int8 p = (slab->objs[idx].size == 1 ? slab->objs[idx].prev : E);
                int8 n = (slab->objs[idx].size == 1 ? slab->objs[idx].next : D);
                set_object_entry(&slab->objs[end_idx], slab->objs[idx].size, p, n);
                // return result
                return (void*) &slab->objects[end_idx + 1];
            }
        }
    }

    return NULL;
}


void*
slab_alloc(uint64 sz)
{
    assert(sz <= (NR_OBJS * OBJECT_SIZE) && sz > 0);
    uint8 nr_objs = ROUNDUP(sz, OBJECT_SIZE) >> OBJECT_SHIFT;
    void* current_alloc = alloc_objs(current, nr_objs);
    if (current_alloc)
        return current_alloc;

    struct slab* ptr_pre = partial;
    struct slab* ptr = get_slab_next(partial);
    // if the head of partial can alloc objs
    if (ptr_pre->sentinel.size >= nr_objs) {
        // swap current and partial head
        set_slab_next(current, ptr);
        set_slab_next(partial, NULL);
        struct slab* tmp = current;
        current = partial;
        partial = tmp;
        // return alloc result
        return alloc_objs(current, nr_objs);
    }

    // find valid slab in partial
    while (ptr != NULL) {
        if (ptr->sentinel.size >= nr_objs) {
            struct slab* ptr_next = get_slab_next(ptr);
            set_slab_next(current, ptr_next);
            set_slab_next(ptr_pre, current);
            current = ptr;
            return alloc_objs(current, nr_objs);
        }
        ptr_pre = ptr;
        ptr = get_slab_next(ptr);
    }

    // if there is none, ask for buddy system
    // add current to the partial list
    set_slab_next(current, partial);
    partial = current;
    partial_len++;
    current = (struct slab*) buddy_alloc(PGSIZE);
    init_slab(current);
    return alloc_objs(current, nr_objs);
}



static inline void 
merge_high(struct slab* slab, int idx, uint8 nr_free)
{
    struct object_entry high = slab->objs[idx + nr_free];
    // set objs[idx]
    set_object_entry(&slab->objs[idx], nr_free + high.size, high.prev, high.next);
    // set prev object_entry and reset its next
    struct object_entry* high_prev = (high.prev == OBJECT_SENTINEL ? &slab->sentinel : &slab->objs[(int)high.prev]);
    high_prev->next = idx;
    // set next object_entry and reset its prev
    struct object_entry* high_next = (high.next == OBJECT_SENTINEL ? &slab->sentinel : &slab->objs[(int)high.next]);
    high_next->prev = idx;
    slab->objs[idx + nr_free + high.size - 1].size += nr_free;
    // clean high object_entry
    set_object_entry(&slab->objs[idx + nr_free], 0, 0, 0);
}

static inline void 
merge_low(struct slab* slab, int idx)
{
    struct object_entry* lower = &slab->objs[idx - slab->objs[idx - 1].size];
    // change lower size
    lower->size += slab->objs[idx].size;
    // remove idx's area from list
    list_remove(slab, idx);
    set_object_entry(&slab->objs[idx], 0, 0, 0);
    // clear old end and set new end
    set_object_entry(&slab->objs[idx - 1], 0, 0, 0);
    lower[lower->size - 1].size = lower->size;
}


void
slab_free(void* addr, uint8 nr_free)
{
    struct slab* slab = SLAB(addr);
    slab->sentinel.size += nr_free;
    assert(slab->sentinel.size <= NR_OBJS);
    // if after free these object, the slab is free, and partial has enough slab
    // ret it to buddy system
    if (slab->sentinel.size == NR_OBJS && partial_len > MIN_PARTIAL) {
        buddy_free((void*) slab, 0);
        partial_len--;
        return;
    }
    int idx = OBJECT_IDX(addr);
    bool merge_h = idx < NR_OBJS - 1 && slab->objs[idx + nr_free].size != 0;
    bool merge_l = idx > 0 \
                && ((slab->objs[idx - 1].prev == E && slab->objs[idx - 1].next == D) \
                || slab->objs[idx - 1].size == 1);
    
    if (!merge_h && !merge_l) {
        // if not merge, create an single area and add it to the end of the list
        int8 last = slab->sentinel.prev;
        struct object_entry* last_entry = (last == OBJECT_SENTINEL ? &slab->sentinel : &slab->objs[(int)last]);
        last_entry->next = idx;
        slab->sentinel.prev = idx;
        set_object_entry(&slab->objs[idx - 1 + nr_free], nr_free, E, D);
        set_object_entry(&slab->objs[idx], nr_free, last, OBJECT_SENTINEL);
    } 
    else if (merge_h) {
        // if there is a higher area to merge, merge it
        merge_high(slab, idx, nr_free);
        // if there is a lower area to merge too, merge again
        if (merge_l) 
            merge_low(slab, idx);
    }
    else {      // the last case is: only need to merge lower block
        struct object_entry* lower = &slab->objs[idx - slab->objs[idx - 1].size];
        // clear old end and set new end
        if (lower->size > 1) 
            set_object_entry(&slab->objs[idx - 1], 0, 0, 0);
        lower->size += nr_free;
        set_object_entry(&slab->objs[idx + nr_free - 1], lower->size, E, D);
    }
}
