#define MAX_ORDER 11  // maximum order of buddy system

#define BLOCK_SIZE(order) (PGSIZE << (order))
#define BLOCK_NPAGES(order) (1 << (order))
#define BUDDY_BLOCK(addr, order) (((uint64) (addr)) ^ (PGSIZE << (order)))
#define BUDDY_LOW(addr, order) (((uint64) (addr)) & ~(((uint64) PGSIZE) << (order)))
#define BUDDY_HIGH(addr, order) (((uint64) (addr)) | (PGSIZE << (order)))

// the block's address must be page-aligned
struct block {
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

