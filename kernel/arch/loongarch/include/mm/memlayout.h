#ifndef __MEMLAYOUT_H__
#define __MEMLAYOUT_H__

#include <platform.h>

#define DMW_MASK    0x9000000000000000UL

#define KERNEL_BASE 0x200000UL
#define RAM_BASE    0x90000000UL

#define KERNELBASE  (DMW_MASK | KERNEL_BASE)
// #define KERNELBASE   DMW_MASK
#define RAMBASE     (DMW_MASK | RAM_BASE)
#define RAMTOP      (RAMBASE + MEMORY_SIZE)
#define PHYSTOP     RAMTOP

#define IN_RAM(addr) (((uint64)(addr) >= RAMBASE) && ((uint64)(addr) < RAMTOP))

#define UART0       (DMW_MASK | UART0_BASE)

#define TRAMPOLINE  0xFFFFFFFFFFFFF000UL
#define TRAPFRAME   (TRAMPOLINE - PGSIZE)

#define KSTACK(n)   (TRAPFRAME - (2 * (n)) * PGSIZE)

#endif