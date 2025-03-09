#define OBJECT_SIZE 64
#define OBJECT_SHIFT 6
#define SLAB_META_DATA_NR_OBJS 3
#define NR_OBJS ((PGSIZE >> OBJECT_SHIFT) - SLAB_META_DATA_NR_OBJS)
#define OBJECT_SENTINEL -1
#define E 0x45
#define D 0x44
#define MIN_PARTIAL 3

#define SLAB(addr) ((struct slab*) PGROUNDDOWN(addr))
#define SLAB_NEXT(slab_addr) ((struct slab*)(((uint64)(slab_addr) & (((1L << 32) - 1L) << 32)) | ((struct slab*)(slab_addr))->next))
#define SLAB_SET_NEXT(slab_pre, slab_next) ((struct slab*)(slab_pre))->next = GET_LOW32(slab_next)
#define OBJECT_IDX(addr) (((uint64)(addr) >> OBJECT_SHIFT) - SLAB_META_DATA_NR_OBJS)
#define NR_FREE_OBJS(slab_addr) (((struct slab*)(slab_addr))->sentinel.size)



#pragma pack(push, 1) 

struct object_entry {
    uint8 size;
    int8 prev;
    int8 next;
};

struct object {
    char buf[OBJECT_SIZE];
};

struct slab {
    uint32 next;
    uint16 reserve;
    struct object_entry sentinel;
    struct object_entry objs[NR_OBJS];
    struct object objects[NR_OBJS];
};

#pragma pack(pop) 



