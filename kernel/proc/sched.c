#include <common.h>
#include <klib.h>
#include <debug.h>
#include <mm/mm.h>
#include <mm/memlayout.h>
#include <irq/interrupt.h>
#include <trap/context.h>
#include <proc/proc.h>
#include <proc/sched.h>
#include <trap/trap.h>
#include <arch.h>
#include <syscall.h>
#include <time.h>

struct proc* proc_list;

extern void timer_intr_on();
extern void timer_intr_off();

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler()
{
    struct proc* p;
    struct cpu* c = mycpu();

    c->proc = NULL;

    intr_on();

    for (;;) {
        timer_intr_off();
        // Avoid deadlock by ensuring that devices can interrupt.
        if (!intr_get()) {
            intr_on();
            intr_off();
        }
        
        for (p = proc_list; p; p = p->next) {
            // lock process
            if (p->state == SLEEPING && p->sleeping_due != -1) {
                if (tick_counter >= p->sleeping_due) {
                    p->sleeping_due = -1;
                    p->state = RUNNABLE;
                }
            }
            if (p->state == RUNNABLE) {
                // switch to this process
                p->state = RUNNING;
                c->proc = p;
                timer_intr_on();
#ifdef ARCH_LOONGARCH   
                set_asid(p->pid);
#endif
                // log("switch to process %s", p->name);
                swtch(&c->context, &p->context);

                timer_intr_off();
                // prev running process is done
                // it should have changed its state brfore swtch back
                c->proc = 0;
            }
        }
    }
}


// Switch to scheduler. Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched()
{
    struct proc* p = myproc();

    if (p->state == RUNNING)
        panic("sched running proc %s", p->name);
    if (intr_get())
        panic("sched interruptable");

    struct cpu* c = mycpu();
    int intena = c->intena;
    swtch(&p->context, &c->context);
    mycpu()->intena = intena;
}


// Give up the CPU for one scheduling round.
void
yield(void)
{
    struct proc *p = myproc();
    p->state = RUNNABLE;
    sched();
}
