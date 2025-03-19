#include <defs.h>
#include <memlayout.h>
#include <debug.h>



void __attribute__((aligned(4)))
kernel_trap()
{
    log("kernel trap");
    uint64 scause = r_scause();
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();

    Assert((sstatus & SSTATUS_SPP) == 0, "kerneltrap: not from supervisor mode");
    Assert((sstatus & SSTATUS_SIE) != 0, "kerneltrap: sie must be 1");

    if (scause == 0x8000000000000005L) {
        // timer interrupt
        timervec();
        w_sip(r_sip() & ~2);
        log("receive timer interrupt");
    }
    else {
        log("unexpected scause %p", (void*)scause);
        log("sepc=%p", (void*)sepc);
        panic("kerneltrap");
    }

}


void
trap_init_hart()
{
    w_stvec((uint64)kernel_trap);
    log("trap init hart");
}