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


/**
 * Initialize a spinlock structure
 * @param lk: Spinlock structure to initialize
 * @param name: Descriptive name for debugging purposes
 */
void    spinlock_init(struct spinlock* lk, const char* name);

/**
 * Check if the current CPU holds a spinlock
 * @param lk: Spinlock to check
 * @return Non-zero if current CPU holds the lock, zero otherwise
 * @note Must be called with interrupts disabled
 */
int     spinlock_holding(struct spinlock* lk);

/**
 * Acquire a spinlock
 * @param lk: Spinlock to acquire
 * @note Disables interrupts during lock acquisition and spins until available
 */
void    spinlock_acquire(struct spinlock* lk);

/**
 * Release a spinlock
 * @param lk: Spinlock to release
 * @note Restores interrupt state from before acquisition
 */
void    spinlock_release(struct spinlock* lk);

#define SPINLOCK_DEFINE(lockname) \
    spinlock_t lockname = { \
        .locked = UNLOCKED, \
        .name = #lockname, \
        .cpu = NULL \
    }

#endif // __SPINLOCK_H__