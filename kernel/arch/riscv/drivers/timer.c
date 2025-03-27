#include <common.h>
#include <riscv.h>
#include <platform.h>
#include <mm/memlayout.h>
#include <debug.h>
#include <trap.h>

extern char timervec[];

#define CLINT VIRT_CLINT
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000L + 8 * (hartid))
#define CLINT_MTIME (CLINT + 0xBFF8L)

#define INTERVAL 10000000L

#define MTIMECMP(hartid) *((uint64*) CLINT_MTIMECMP(hartid))
#define MTIME *((uint64*) CLINT_MTIME)

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

    log("mstatus: %p", (void*)r_mstatus());
    log("mie: %p", (void*)r_mie());
    log("mtvec: %p", (void*)r_mtvec());
    log("mideleg: %p", (void*)r_mideleg());
    log("medeleg: %p", (void*)r_medeleg());
}

void
timer_interrupt_handler()
{
    log("receive timer interrupt");
    // clear timer interrupt
    w_sip(r_sip() & ~(1 << SUPERVISOR_SOFTWARE_INTERRUPT));
}