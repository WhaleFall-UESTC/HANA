#define MAX_ORDER 11  // maximum order of buddy system
#define BUDDY_MAGIC 0x6275646479000000L // 'buddy'
// struct list_head {
//     struct list_head* prev;
//     struct list_head* next; 0x6275646479
// };

struct free_area {
    struct block free_list;
    uint64 nr_free;     // number of free blocks in the area
};

struct zone {
    struct free_area free_area[MAX_ORDER];
};


// the block's address must be page-aligned
struct block {
    int order;
    struct block* prev;
    struct block* next;
}
