#define XLEN 64
#define NCPU 1
#define MEMORY_SIZE_SHIFT 27
#define MEMORY_SIZE (1 << MEMORY_SIZE_SHIFT) // 128MB

#define PGSIZE 4096
#define PGSHIFT 12

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))
#define IS_PGALIGNED(a) ((a) & 0xfff)

#define NULL 0

// typedefs
typedef unsigned long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

static inline int
cpuid()
{
    int id;
    asm volatile("mv %0, tp" : "=r" (id) );
    return id;
}
