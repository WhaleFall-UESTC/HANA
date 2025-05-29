#include <common.h>
#include <arch.h>
#include <platform.h>
#include <mm/memlayout.h>
#include <debug.h>
#include <trap/context.h>
#include <trap/trap.h>
#include <irq/interrupt.h>
#include <proc/proc.h>
#include <proc/sched.h>

extern char timervec[];

#define CLINT VIRT_CLINT
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000L + 8 * (hartid))
#define CLINT_MTIME (CLINT + 0xBFF8L)

#define INTERVAL 10000000L

#define MTIMECMP(hartid) *((uint64*) CLINT_MTIMECMP(hartid))
#define MTIME *((uint64*) CLINT_MTIME)

#ifdef BIOS_SBI
#include <sbi/sbi.h>

static inline void update_time() {
    sbi_set_timer(r_time() + INTERVAL);
}

void timer_interrupt_handler()
{
    log("receive timer interrupt");
    update_time();

    struct proc* p = myproc();
    if (p && p->state == RUNNING)
        yield();
}

void 
timer_init() 
{
    update_time();
    w_sie(r_sie() | SIE_STIE);
}

#else

void
timer_init()
{
    // Initialize the timer
    // set first timer to interrupt (1s)
    MTIMECMP(0) = MTIME + INTERVAL;
    
    // enable machine interrupt
    w_mstatus(r_mstatus() | MSTATUS_MIE);
    // enable machine timer interrupt
    w_mie(r_mie() | MIE_MTIE);
    // set machine trap vector
    w_mtvec((uint64)timervec); 

    debug("mstatus: %p", (void*)r_mstatus());
    debug("mie: %p", (void*)r_mie());
    debug("mtvec: %p", (void*)r_mtvec());
    debug("mideleg: %p", (void*)r_mideleg());
    debug("medeleg: %p", (void*)r_medeleg());
}

void
timer_interrupt_handler()
{
    log("receive timer interrupt");
    // clear timer interrupt
    w_sip(r_sip() & ~(0x2));

    struct proc* p = myproc();
    if (p && p->state == RUNNING)
        yield();
}

#endif