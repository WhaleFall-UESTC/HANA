#ifndef __SLAB_H__
#define __SLAB_H__

#define OBJECT_SIZE 64
#define OBJECT_SHIFT 6

// 6
struct object_entry {
    uint8 ls, rs;
    uint8 lp, len, mxlen;
    uint8 h;
};

// 64
struct object {
    char buf[OBJECT_SIZE];
};

#define OTHER_SIZE (3 * sizeof(uint64) + 3 * sizeof(uint8))
#define OBJ_SIZE (sizeof(uint8) + sizeof(struct object) + sizeof(struct object_entry))
#define NR_OBJS ((PGSIZE - OTHER_SIZE) / OBJ_SIZE)
#define OBJ_OFFSET (64 - NR_OBJS)
#define FOO_SIZE (PGSIZE - OTHER_SIZE - OBJ_SIZE * NR_OBJS)

#define MIN_NODE_CNT 3

#define SLAB(addr) ((struct slab*) PGROUNDDOWN(addr))
#define OBJECT_IDX(addr) ((((uint64)(addr) - PGROUNDDOWN(addr)) >> OBJECT_SHIFT) - OBJ_OFFSET)

#define NULLPTR -1

#pragma pack(push, 1) 

struct slab {
    struct slab* fa, ls, rs; // 24
    uint8 mxlen; // 1

    uint8 rt, tp; // 2
    uint8 nd_stack[NR_OBJS];

    struct object_entry objs_info[NR_OBJS]; 

    uint8 foo[FOO_SIZE];

    struct object objects[NR_OBJS];
};

#pragma pack(pop)

/* mm/slab.c */
void            slab_init();
void*           slab_alloc(uint64 sz);
void            slab_free(void* addr);


#endif // __SLAB_H__