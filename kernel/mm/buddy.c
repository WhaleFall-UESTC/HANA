#include <common.h>
#include <klib.h>
#include <debug.h>
#include <mm/buddy.h>
#include <mm/memlayout.h>

extern struct page* pages;
struct zone zone;

static inline int 
get_order(uint64 size)
{
    size >>= PGSHIFT;
    int order = 0;
    while (size > (1 << order))
        order++;
    return order;
}


static inline void
add_first(uint64 addr, int order)
{
    assert(IS_PGALIGNED(addr));
    struct block* new_block = (struct block*) addr;
    new_block->order = order;

    struct block* first_block = zone.free_area[order].free_list.next;
    zone.free_area[order].free_list.next = new_block;
    new_block->prev = &zone.free_area[order].free_list;
    new_block->next = first_block;
    first_block->prev = new_block;

    zone.free_area[order].nr_free++;
}


static inline void
add_last(uint64 addr, int order)
{
    assert(IS_PGALIGNED(addr));
    struct block* new_block = (struct block*) addr;
    new_block->order = order;
    // unsafe design
    new_block->magic = BUDDY_MAGIC;

    struct block* last_block = zone.free_area[order].free_list.prev;
    zone.free_area[order].free_list.prev = new_block;
    new_block->next = &zone.free_area[order].free_list;
    new_block->prev = last_block;
    last_block->next = new_block;

    zone.free_area[order].nr_free++;
}


static inline void
remove(struct block* remove_block)
{
    struct block* prev_block = remove_block->prev;
    struct block* next_block = remove_block->next;
    prev_block->next = next_block;
    next_block->prev = prev_block;
    // nop, you should not do this
    // buddy_free(remove_block, remove_block->order);
    zone.free_area[remove_block->order].nr_free--;
    // need to clear this block
    // once allocated but not used, buddy_free will free it
    remove_block->order = -1;
    remove_block->magic = 0;
}


// end should be page-aligned
void
buddy_init(uint64 start, uint64 end)
{
    uint64 s = PGROUNDUP(start);
    uint64 e = PGROUNDDOWN(end);
    debug("buddy alloc %lx - %lx", s, e);
    uint64 npages = ((e - s) >> PGSHIFT);
    uint64 npages_copy = npages;
    debug("npages: %#lx, size is: %#lx", npages, npages << PGSHIFT);
    memset(&zone, 0, sizeof(struct zone));
    Assert(s <= e, "Get s %lx, but e %lx", s, e);
    // log("compute the number of init blocks for each free_area");

    // compute the number of init blocks for each free_area
    uint64 square_npages = (MAX_ORDER * BLOCK_NPAGES(MAX_ORDER - 1));
    if (npages > square_npages) {
        npages -= square_npages;
        for (int i = 0; i < MAX_ORDER; i++) {
            zone.free_area[i].nr_free = (1 << (MAX_ORDER - 1 - i));
        }
    }

    for (int i = MAX_ORDER - 1; i >= 0; i--) {
        zone.free_area[i].nr_free += (npages >> i);
        npages &= ((1 << i) - 1);

        // init link list
        zone.free_area[i].free_list.prev = &zone.free_area[i].free_list;
        zone.free_area[i].free_list.next = &zone.free_area[i].free_list;
    }

    for (int i = 0; i < MAX_ORDER; i++)
        npages_copy -= BLOCK_NPAGES(i) * zone.free_area[i].nr_free;
    assert(npages_copy == 0);
    // log("allocate blocks");

    // allocate blocks
    for (int i = 0; i < MAX_ORDER; i++) {
        uint64 current_block_size = BLOCK_SIZE(i);
        int nr_alloc = zone.free_area[i].nr_free;
        zone.free_area[i].nr_free = 0;
        // log("order %d get %d block, each block %d pages, start at %p", i, nr_alloc, (int)(current_block_size >> PGSHIFT), (void*)s);
        for (int j = 0; j < nr_alloc; j++) {
            // log("alloc %p", (void*) s);
            add_last(s, i);
            s += current_block_size;
        }
    }

    assert(s == e);
}


// 1. find a bigger block and split it
// 2. split to 1/2 + 1/4 ... + 1/2^n + 1/2^n
// 3. a 1/2^n will be stored in list_free_list, return the other
static inline void*
require(int order)
{
    for (int i = order + 1; i < MAX_ORDER; i++) {
        if (zone.free_area[i].nr_free) {
            struct block* free_block = zone.free_area[i].free_list.prev;
            remove(free_block);
            uint64 addr = (uint64) free_block;
            for (int j = i - 1; j >= order; j--) {
                add_first(addr, j);
                addr += BLOCK_SIZE(j);
            }
            return (void*) addr;
        } 
        else continue;
    }
    return NULL;
}


void*
buddy_alloc(uint64 sz)
{
    assert(sz > 0);
    uint64 size = PGROUNDUP(sz);
    int order = get_order(size);
    assert(sz <= (PGSIZE << order));

    if (zone.free_area[order].nr_free) {
        struct block* free_block = zone.free_area[order].free_list.prev;
        remove(free_block);
        SET_PAGE_ORDER(free_block, pages, order);
        return (void*) free_block;
    }
    else {
        void* require_result = require(order);
        if (require_result != NULL) {
            SET_PAGE_ORDER(require_result, pages, order);
            return require_result;
        } else TODO();    // allocator
        panic("Memory is out");
    }

    return NULL;
}


void
buddy_free_helper(void* addr, int order)
{
    struct block* buddy = (struct block*) BUDDY_BLOCK(addr, order);
    if (order < MAX_ORDER - 1 
        && buddy->magic == BUDDY_MAGIC && buddy->order == order  
        && IN_RAM(buddy->prev) && IN_RAM(buddy->next))
    {
        remove(buddy);
        // memset((void*) BUDDY_HIGH(addr, order), 0, sizeof(struct block));
        buddy_free_helper((void*) BUDDY_LOW(addr, order), order + 1);
    }
    else {
        add_last((uint64) addr, order);
    }
}

void
buddy_free(void* addr, int order)
{
    // assert(GET_PAGE_ORDER(addr, pages) == order);
    SET_PAGE_ORDER(addr, pages, 0);
    buddy_free_helper(addr, order);
}
