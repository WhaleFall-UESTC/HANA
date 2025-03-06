/* kernel va and pa will ba the same */
#define KERNELBASE  0x800000000L
#define KERNELTOP   0x864000000L

#define TRAMPOLINE  (MAXVA - PGSIZE)
#define TRAPFRAM    (TRAMPOLINE - PGSIZE)

#define KSTACK(n)   (TRAPFRAM- (2 * (n) + 1) * PGSIZE)

