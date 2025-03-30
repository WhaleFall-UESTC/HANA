#ifndef __SYNC_H__
#define __SYNC_H__

#include <common.h>
#include <irq/interrupt.h>

typedef uint32 spinsem_t;

#define DECLARE_SPINSEM(name, init) spinsem_t name = init
#define INIT_SPINSEM(addr, val)     *(addr) = (val)

#define _spin_acquire(inputaddr)                                               \
    __asm__ __volatile__("1:                                                  \n\t" \
                         "    lr.w   a0, %[addr]                              \n\t" \
                         "    beqz   a0, 1b                                   \n\t" \
                         "    addi   a0, a0, -1                               \n\t" \
                         "    sc.w   a1, a0, %[addr]                          \n\t" \
                         "    bnez   a1, 1b                                   \n\t" \
                         "    fence   rw, rw                                  \n\t" \
                         :                                                     \
                         : [addr] "m" (*(inputaddr))                           \
                         : "a0", "a1", "memory")

static inline void spin_acquire_irqsave(spinsem_t *sem, int *flags) {
    int sie;
    irq_save(flags);
    _spin_acquire(sem);
}

#define _spin_release(inputaddr)                                               \
    __asm__ __volatile__("    fence   rw, rw                                  \n\t" \
                         "1:                                                  \n\t" \
                         "    lr.w   a0, %[addr]                              \n\t" \
                         "    addi   a0, a0, 1                                \n\t" \
                         "    sc.w   a1, a0, %[addr]                          \n\t" \
                         "    bnez   a1, 1b                                   \n\t" \
                         :                                                     \
                         : [addr] "m" (*(inputaddr))                           \
                         : "a0", "a1", "memory")

static inline void spin_release_irqrestore(spinsem_t *sem, int *flags) {
    _spin_release(sem);
    irq_store(flags);
}

#endif // __SYNC_H__