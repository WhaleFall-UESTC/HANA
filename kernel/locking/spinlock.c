#include <common.h>
#include <klib.h>
#include <debug.h>

#ifdef ARCH_RISCV
#include <riscv.h>
#endif

#include <locking/spinlock.h>
#include <trap/context.h>
#include <proc/proc.h>


void 
init_lock(struct spinlock* lk, char* name) 
{
    lk->locked = UNLOCKED;
    lk->name = name;
    lk->cpu = NULL;
}


void
push_off()
{
    int old_intr_status = intr_get();
    intr_off();
    
    struct cpu* c = mycpu();
    if (c->noff == 0)
        c->intena = old_intr_status;
    c->noff++;
}


void 
pop_off()
{
    Assert(!intr_get(), "interruptible");
    struct cpu* c = mycpu();
    assert(c->noff > 0);
    if (--c->noff == 0 && c->intena)
        intr_on();
}


// check if this cpu is holding the lock
// intr must br off
int
holding(struct spinlock* lk)
{
    return (lk->locked && lk->cpu == mycpu());
}


// acquire the lock, spining until lock is required
// turn off intr at first 
// and turn on intr at the end of release
void 
acquire(struct spinlock* lk)
{
    // disable intr, avoid deadlock
    push_off();
    Assert(!holding(lk), "cpu%d has hold lock %s", CPUID(lk->cpu), lk->name);

    while(__sync_lock_test_and_set(&lk->locked, LOCKED) != UNLOCKED)
        ;

    __sync_synchronize();

    lk->cpu = mycpu();
}


// release the lock
void
release(struct spinlock* lk)
{
    Assert(holding(lk), "cpu%d try to release lock %s, which is hold by cpu%d", (int)r_tp(), lk->name, CPUID(lk->cpu));

    lk->cpu = NULL;

    __sync_synchronize();

    __sync_lock_release(&lk->locked);

    pop_off();
}
