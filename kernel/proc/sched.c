#include <common.h>
#include <klib.h>
#include <debug.h>
#include <mm/mm.h>
#include <mm/memlayout.h>
#include <context.h>
#include <proc/proc.h>
#include <proc/sched.h>
#include <trap.h>

#ifdef ARCH_RISCV
#include <riscv.h>
#endif

struct proc* proc_list;


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

    for (;;) {
        intr_on();
        for (p = proc_list; p; p = p->next) {
            // lock process
            if (p->state == RUNNABLE) {
                // switch to this process
                p->state = RUNNING;
                c->proc = p;
                swtch(&c->context, &p->context);

                // prev running process is done
                // it should have changed its state brfore swtch back
                c->proc = 0;
            }
        }
    }
}



