#ifndef __MEMLAYOUT_H__
#define __MEMLAYOUT_H__

#include <platform.h>


#ifdef ARCH_RISCV
/* kernel va and pa will be the same */
#define KERNELBASE  0x80200000L
#elif defined(ARCH_LOONGARCH)
#define KERNELBASE  0x9000000000000000UL
#define MAXVA       (1UL << 63)
#endif

// qemu-system-riscv64 virt has limit
#define PHYSTOP     0x88000000L

#define IN_KERNEL(addr) (((uint64)(addr)) >= KERNELBASE && ((uint64)(addr)) < PHYSTOP)

#define TRAMPOLINE  (MAXVA - PGSIZE)
#define TRAPFRAME   (TRAMPOLINE - PGSIZE)

#define KSTACK(n)   (TRAPFRAME - ((KSTACK_SIZE + PGSIZE) * (n)))


#define UART0   VIRT_UART0

#define VIRTIO0 VIRT_VIRTIO

#define CLINT   VIRT_CLINT

#define PLIC    VIRT_PLIC

#endif // __MEMLAYOUT_H__