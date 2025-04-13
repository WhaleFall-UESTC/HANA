#include <common.h>
#include <klib.h>
#include <debug.h>

#ifdef ARCH_RISCV
#include <riscv.h>
#endif

#include <irq/interrupt.h>
#include <locking/spinlock.h>
#include <trap/context.h>
#include <proc/proc.h>
#include <irq/interrupt.h>


void 
spinlock_init(struct spinlock* lk, char* name) 
{
    lk->locked = UNLOCKED;
    lk->name = name;
    lk->cpu = NULL;
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
}
