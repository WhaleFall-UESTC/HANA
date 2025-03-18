#include <defs.h>
#include <platform.h>
#include <memlayout.h>
#include <debug.h>

#define CLINT VIRT_CLINT
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000L + 8 * (hartid))
#define CLINT_MTIME (CLINT + 0xBFF8L)

#define MTIMECMP(hartid) *((uint64*) CLINT_MTIMECMP(hartid))
#define MTIME *((uint64*) CLINT_MTIME)

void
timervec()
{

}

void
timer_init()
{
    // Initialize the timer
    // set first timer to interrupt (1s)
    MTIMECMP(0) = MTIME + 10000000L;
    // enable machine interrupt
    w_mstatus(r_mstatus() | MSTATUS_MIE);
    // enable machine timer interrupt
    w_mie(r_mie() | MIE_MTIE);
    // set machine trap vector
    w_mtvec((uint64)timervec);
}