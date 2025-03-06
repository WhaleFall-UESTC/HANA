#include <defs.h>
#include <common.h>
#include <debug.h>
#include "buddy.h"

#define BLOCK_SIZE(order) (PGSIZE << order)

struct zone zone;

static inline int 
get_order(uint64 size)
{
    size >>= PGSHIFT;
    int order = 0;
    while (size >>= 1)
        order++;
    return order;
}

static inline void*
dequeue(int order)
{
    struct free_list* fl = zone.free_area[order].head;
    zone.free_area[order].head = fl->next;
    return (void*) fl;
}

static inline void
enqueue(int order, void* addr)
{
    struct free_list* fl = (struct free_list*) addr;
    addr->next = zone.free_area[order].head;
    zone.free_area[order].head = fl;
}

static inline void*
require(int order)
{
    int i;
    for (i = order + 1; i < MAX_ORDER; i++)
        if (zone.free_area[i].head)
            break;
    if (i == MAX_ORDER)
        panic("No free physics memory");

    uint64 block_addr = (uint64) dequeue(i);
    uint64 block_size = BLOCK_SIZE(i);

    for (int j = i - 1; j >= order; j--) {
        block_size >>= 1;
        // split form a bigger block, no buddy block, just enqueue
        enqueue(j, (void*)(block_addr));
        block_addr += block_size;
    }

    return (void*) block_addr;
}


void*
buddy_alloc(uint64 sz)
{
    uint64 size = PGROUNDUP(sz);
    int order = get_order(size);

    if (zone.free_area[order].head) {
        return dequeue(order);
    }
    else {
        
    }
    
    return NULL;
}


void
buddy_free(void* addr)
{

}












// end should be page-aligned
void 
buddy_init(uint64 start, uint64 end)
{
    uint64 s = PGROUNDUP(start);
    uint64 e = PGROUNDDOWN(end);
    uint npages = ((e - s) >> PGSHIFT);
    zone.npages = npages;

    int length[MAX_ORDER];
    memset(&zone, 0, sizeof(struct zone));
    memset(length, 0, MAX_ORDER * sizeof(int));

    // compute the number of pages in each free area
    for (int i = MAX_ORDER - 1; i > 0; i--) {
        uint square_npages = SQUARE_NPAGES(i);
        uint nblocks = npages / square_npages;
        if (nblocks > 0) {
            for (int j = i; j >= 0; j--)
                length[j] += (nblocks << (i - j));
            npages -= nblocks * square_npages;
            if (npages == 0) 
                break;
        }
    }

    if (npages > 0) 
        length[0] += npages;

    // create free list
    for (int i = 0; i < MAX_ORDER; i++) {
        uint len = length[i];
        if (len == 0)
            continue;
        uint block_size = (PGSIZE << i);

        uint64 s_prev = s;
        s += block_size * len;
        for (uint64 ptr = s - block_size; ptr >= s_prev; ptr -= block_size) {
            struct free_list* fl = (struct free_list*) ptr;
            fl->next = zone.free_area[i].head;
            zone.free_area[i].head = fl;
        }

        assert(s <= e);
    }
}
