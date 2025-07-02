#ifndef __BUDDY_H__
#define __BUDDY_H__

#define MAX_ORDER 11  // maximum order of buddy system
#define BUDDY_MAGIC 0x42554459

#define BLOCK_SIZE(order) (PGSIZE << (order))
#define BLOCK_NPAGES(order) (1 << (order))
#define BUDDY_BLOCK(addr, order) (((uint64) (addr)) ^ (PGSIZE << (order)))
#define BUDDY_LOW(addr, order) (((uint64) (addr)) & ~(((uint64) PGSIZE) << (order)))
#define BUDDY_HIGH(addr, order) (((uint64) (addr)) | (PGSIZE << (order)))

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


/* mm/buddy.c */
/**
 * buddy system 的初始化函数
 * @param start 可分配空间的起始地址
 * @param end   可分配空间的结束地址
 */
void            buddy_init(uint64 start, uint64 end);

/**
 * buddy system 的分配函数
 * @param sz 要分配的大小（字节）
 * @return buddy system 分配空间的指针
 */
void*           buddy_alloc(uint64 sz);

/**
 * buddy system 的释放函数
 * @param sz 要分配的 buddy 块以及其 order
 */
void            buddy_free(void* addr, int order);

#endif // __BUDDY_H__

