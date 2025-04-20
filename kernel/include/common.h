#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#define XLEN 64
#define NCPU 1
#define MEMORY_SIZE_SHIFT 27
#define MEMORY_SIZE (1 << MEMORY_SIZE_SHIFT) // 128MB

#define PGSIZE 4096
#define PGSHIFT 12

#define KSTACK_SIZE PGSIZE

#define PGROUNDUP(sz)  (((uint64)(sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((uint64)(a)) & ~(PGSIZE-1))
#define IS_PGALIGNED(a) (!((uint64)(a) & 0xfff))
#define PGOFFSET(a)    (((uint64)(a)) & (PGSIZE - 1))

#define GET_LOW32(a) ((uint64)(a) & 0xffffffffL)
#define GET_HIGH32(a) GET_LOW32(a >> 32)
#define ROUNDUP(sz, align_size)  (((uint64)(sz)+align_size-1) & ~(align_size-1))
#define ROUNDDOWN(a, align_size) (((uint64)(a)) & ~(align_size-1))

#define ALIGN(x, a) __ALIGN_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))

#ifdef __WRITE32
#define WRITE32 __WRITE32
#else
#define WRITE32(_reg, _val) \
    do { \
        register uint32 __myval__ = (_val); \
        *(volatile uint32 *)&(_reg) = __myval__; \
    } while (0)
#endif

#ifdef __READ32
#define READ32 __READ32
#else
#define READ32(_reg) (*((volatile uint32 *)(&(_reg))))
#endif

#ifdef __READ64
#define READ64 __READ64
#else
#define READ64(_reg) (*((volatile uint64 *)(&(_reg))))
#endif

#define KALLOC(type, var) type* var = (type*) kalloc(sizeof(type))

#define nr_elem(x) (sizeof(x) / sizeof(x[0]))

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

typedef unsigned long sector_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long off_t;
typedef unsigned long time_t;

typedef unsigned short umode_t;

#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define BUILD_BUG_ON_ZERO(e) ((int)(sizeof(struct { int:(-!!(e)); })))

#define __force	__attribute__((force))

#define metamacro_head(...) metamacro_head_(__VA_ARGS__, 0)
#define metamacro_concat_(A, B) A ## B
#define metamacro_concat(A, B) metamacro_concat_(A, B)
#define metamacro_at(N,...) metamacro_concat(metamacro_at, N)(__VA_ARGS__)
#define metamacro_inc(VAL) metamacro_at(VAL, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21)
#define metamacro_dec(VAL) metamacro_at(VAL, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19)

#define metamacro_head_(First, ...) First

#define metamacro_at0(...) metamacro_head(__VA_ARGS__)
#define metamacro_at1(_0, ...) metamacro_head(__VA_ARGS__)
#define metamacro_at2(_0, _1, ...) metamacro_head(__VA_ARGS__)
#define metamacro_at3(_0, _1, _2, ...) metamacro_head(__VA_ARGS__)
#define metamacro_at4(_0, _1, _2, _3, ...) metamacro_head(__VA_ARGS__)
#define metamacro_at5(_0, _1, _2, _3, _4, ...) metamacro_head(__VA_ARGS__)
#define metamacro_at6(_0, _1, _2, _3, _4, _5, ...) metamacro_head(__VA_ARGS__)

#endif // __COMMON_H__

