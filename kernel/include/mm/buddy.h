#ifndef __BUDDY_H__
#define __BUDDY_H__

#define MAX_ORDER 11  // maximum order of buddy system
#define BUDDY_MAGIC 0x42554459

#define BLOCK_SIZE(order) (PGSIZE << (order))
#define BLOCK_NPAGES(order) (1 << (order))
#define BUDDY_BLOCK(addr, order) (((uint64) (addr)) ^ (PGSIZE << (order)))
#define BUDDY_LOW(addr, order) (((uint64) (addr)) & ~(((uint64) PGSIZE) << (order)))
#define BUDDY_HIGH(addr, order) (((uint64) (addr)) | (PGSIZE << (order)))

#define GET_PAGE(addr, pages) (&pages[((uint64)addr - (uint64)pages) >> PGSHIFT])
#define GET_PAGE_ORDER(addr, pages) (pages[((uint64)addr - (uint64)pages) >> PGSHIFT].order)
#define SET_PAGE_ORDER(addr, pages, new_order) pages[((uint64)addr - (uint64)pages) >> PGSHIFT].order = new_order

// the block's address must be page-aligned
struct block {
    uint64 magic;
    int order;
    struct block* prev;
    struct block* next;
};

struct free_area {
    struct block free_list;
    uint64 nr_free;     // number of free blocks in the area
};

struct zone {
    struct free_area free_area[MAX_ORDER];
};

#pragma pack(push, 1)

struct page {
    uint order : 4;
    uint flag : 4;
};

#pragma pack(pop)


/* mm/buddy.c */
void            buddy_init(uint64 start, uint64 end);
void*           buddy_alloc(uint64 sz);
void            buddy_free(void* addr, int order);

#endif // __BUDDY_H__

