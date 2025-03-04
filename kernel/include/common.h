
#define PGSIZE 4096

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
