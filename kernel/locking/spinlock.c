/**
 * This code is from xv6 (MIT License)
 * Original source: https://github.com/mit-pdos/xv6-riscv/tree/riscv/kernel/spinlock.c
 * Copyright (c) 2006-2024 Frans Kaashoek, Robert Morris, Russ Cox,
 *                      Massachusetts Institute of Technology
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#include <common.h>
#include <klib.h>
#include <debug.h>
#include <arch.h>

#include <irq/interrupt.h>
#include <locking/spinlock.h>
#include <trap/context.h>
#include <proc/proc.h>
#include <irq/interrupt.h>

void 
spinlock_init(struct spinlock* lk, const char* name) 
{
    lk->locked = UNLOCKED;
    lk->cpu = NULL;

    strncpy(lk->name, name, SPINLOCK_NAME_MAX_LEN);
}


// check if this cpu is holding the lock
// intr must br off
int
spinlock_holding(struct spinlock* lk)
{
    return (lk->locked && lk->cpu == mycpu());
}


// acquire the lock, spining until lock is required
// turn off intr at first 
// and turn on intr at the end of release
void 
spinlock_acquire(struct spinlock* lk)
{
    // disable intr, avoid deadlock
    irq_pushoff();
    // intr_off();
    Assert(!spinlock_holding(lk), "cpu%d has hold lock %s", CPUID(lk->cpu), lk->name);

    while(__sync_lock_test_and_set(&lk->locked, LOCKED) != UNLOCKED)
        ;

    __sync_synchronize();

    lk->cpu = mycpu();
}


// release the lock
void
spinlock_release(struct spinlock* lk)
{
    Assert(spinlock_holding(lk), "cpu%d try to release lock %s, which is hold by cpu%d", (int)r_tp(), lk->name, CPUID(lk->cpu));

    lk->cpu = NULL;

    __sync_synchronize();

    __sync_lock_release(&lk->locked);

    irq_popoff();
    // intr_on();
}
