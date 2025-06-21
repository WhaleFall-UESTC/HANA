#ifndef __SLAB_H__
#define __SLAB_H__

#define OBJECT_SIZE 64
#define OBJECT_SHIFT 6
#define SLAB_META_DATA_NR_OBJS 3
#define NR_OBJS ((PGSIZE >> OBJECT_SHIFT) - SLAB_META_DATA_NR_OBJS)
#define OBJECT_SENTINEL (uint8)-1
#define E 0x45
#define D 0x44
#define A 0x41
#define L 0x4c
#define MIN_PARTIAL 3

#define SLAB(addr) ((struct slab*) PGROUNDDOWN(addr))
#define OBJECT_IDX(addr) ((((uint64)(addr) - PGROUNDDOWN(addr)) >> OBJECT_SHIFT) - SLAB_META_DATA_NR_OBJS)


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
    uint16 next_low;
    uint8  next_high;
    uint16 prev_low;
    uint8  prev_high;

    struct object_entry sentinel;
    struct object_entry objs[NR_OBJS];
    
    struct object objects[NR_OBJS];
};

#pragma pack(pop) 

static inline struct slab*
get_slab_next(struct slab* s) 
{
    uint32 next_low = (((s->next_high << 16) | s->next_low) << 12);
    return (struct slab *) ((((uint64) s) & 0xffffffff00000000) | next_low);
}

static inline struct slab*
get_slab_prev(struct slab* s) 
{
    uint32 prev_low = (((s->prev_high << 16) | s->prev_low) << 12);
    return (struct slab *) ((((uint64) s) & 0xffffffff00000000) | prev_low);
}

static inline void
set_slab_next(struct slab* s, struct slab* next) 
{
    uint64 next_addr = (uint64) next;
    s->next_low = (uint16) ((next_addr >> 12) & 0xffff);
    s->next_high = (uint8) ((next_addr >> (12 + 16)) & 0xf);
}

static inline void
set_slab_prev(struct slab* s, struct slab* prev) 
{
    uint64 prev_addr = (uint64) prev;
    s->prev_low = (uint16) ((prev_addr >> 12) & 0xffff);
    s->prev_high = (uint8) ((prev_addr >> (12 + 16)) & 0xf);
}

// static inline struct slab*
// get_slab_next(struct slab* s) 
// {
//     uint64 slab_addr = (uint64) s;
//     slab_addr &= 0xffffffff00000000L;
//     slab_addr |= s->next;
//     return (struct slab*) slab_addr;
// }

// static inline void 
// set_slab_next(struct slab* prev, struct slab* next)
// {
//     prev->next = (uint32)((uint64)next & 0xffffffffL);
// }

static inline uint8
nr_free_objs(struct slab* s)
{
    return s->sentinel.size;
}

static inline void
set_object_entry(struct object_entry* entry, uint8 size, int8 prev, int8 next)
{
    entry->size = size;
    entry->prev = prev;
    entry->next = next;
}

static inline void 
slab_list_remove(struct slab* slab, int idx)
{
    int8 idx_prev = slab->objs[idx].prev;
    int8 idx_next = slab->objs[idx].next;
    struct object_entry* prev_entry = (idx_prev == OBJECT_SENTINEL ? &slab->sentinel : &slab->objs[(int)idx_prev]);
    struct object_entry* next_entry = (idx_next == OBJECT_SENTINEL ? &slab->sentinel : &slab->objs[(int)idx_next]);
    prev_entry->next = idx_next;
    next_entry->prev = idx_prev;
}


/* mm/slab.c */
void            slab_init();
void*           slab_alloc(uint64 sz);
void            slab_free(void* addr, uint8 nr_free);


#endif // __SLAB_H__