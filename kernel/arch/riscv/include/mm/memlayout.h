#ifndef __MEMLAYOUT_H__
#define __MEMLAYOUT_H__

#include <platform.h>

/* kernel va and pa will be the same */
#define KERNELBASE  0x80200000L

// qemu-system-riscv64 virt has limit
#define PHYSTOP     0x88000000L

#define IN_RAM(addr) (((uint64)(addr)) >= KERNELBASE && ((uint64)(addr)) < PHYSTOP)

#define TEST_SPACE  (MAXVA - PGSIZE)
#define TRAMPOLINE  (MAXVA - 2*PGSIZE)
#define TRAPFRAME   (TRAMPOLINE - PGSIZE)

#define KSTACK(n)   (TRAPFRAME - ((KSTACK_SIZE + PGSIZE) * (n)))

#define KERNEL_VA2PA(va) va 
#define KERNEL_PA2VA(pa) pa

#define UART0   VIRT_UART0

#define VIRTIO0 VIRT_VIRTIO

#define CLINT   VIRT_CLINT

#define PLIC    VIRT_PLIC

#endif // __MEMLAYOUT_H__