#define MAX_ORDER 11  // maximum order of buddy system

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

