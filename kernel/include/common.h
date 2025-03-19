#include <stdbool.h>

#define XLEN 64
#define NCPU 1
#define MEMORY_SIZE_SHIFT 27
#define MEMORY_SIZE (1 << MEMORY_SIZE_SHIFT) // 128MB

#define PGSIZE 4096
#define PGSHIFT 12

#define KSTACK_SIZE PGSIZE*2

#define PGROUNDUP(sz)  (((uint64)(sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((uint64)(a)) & ~(PGSIZE-1))
#define IS_PGALIGNED(a) (!((uint64)(a) & 0xfff))
#define PGOFFSET(a)    (((uint64)(a)) & (PGSIZE - 1))

#define GET_LOW32(a) ((uint64)(a) & 0xffffffffL)
#define GET_HIGH32(a) GET_LOW32(a >> 32)
#define ROUNDUP(sz, align_size)  (((uint64)(sz)+align_size-1) & ~(align_size-1))
#define ROUNDDOWN(a, align_size) (((uint64)(a)) & ~(align_size-1))

// typedefs
typedef unsigned long uint64;
typedef unsigned int uint32;
typedef unsigned int uint;
typedef unsigned short uint16;
typedef unsigned char uint8;

typedef long int64;
typedef int int32;
typedef short int16;
typedef char int8;

