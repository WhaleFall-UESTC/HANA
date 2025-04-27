#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include <common.h>

#define SPINLOCK_NAME_MAX_LEN 32

struct spinlock {
    uint32 locked;

    // For debugging
    char name[SPINLOCK_NAME_MAX_LEN];
    struct cpu *cpu;    // which cpu holding this lock
};

typedef struct spinlock spinlock_t;

#define UNLOCKED    0
#define LOCKED      1

void    spinlock_init(struct spinlock* lk, const char* name);
void    spinlock_acquire(struct spinlock* lk);
void    spinlock_release(struct spinlock* lk);
int     spinlock_holding(struct spinlock* lk);


#endif // __SPINLOCK_H__