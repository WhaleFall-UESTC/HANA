#include <platform.h>

/* kernel va and pa will ba the same */
#define KERNELBASE  0x80000000L
#define PHYSTOP   0x86400000L

#define TRAMPOLINE  (MAXVA - PGSIZE)
#define TRAPFRAM    (TRAMPOLINE - PGSIZE)

#define KSTACK(n)   (TRAPFRAM- (2 * (n) + 1) * PGSIZE)


#define UART0   VIRT_UART0

#define VIRTIO0 VIRT_VIRTIO

#define CLINT   VIRT_CLINT

#define PLIC    VIRT_PLIC
