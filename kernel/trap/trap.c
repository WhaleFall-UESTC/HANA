#include <defs.h>
#include <memlayout.h>
#include <debug.h>

extern char kernelvec[];

#define PANIC(type) do { \
    log("unexpected " #type " irq: %d", irq); \
    log("scause: %p", (void*)scause); \
    log("sepc: %p", (void*)sepc); \
    log("sstatus: %p", (void*)sstatus); \
    panic("kernel_trap"); \
} while(0)


void
kernel_trap()
{
    log("kernel trap");
    uint64 scause = r_scause();
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();

    if (scause & (1UL << 63)) {
        // interrupt, not exception
        int irq = scause & 0xfff;
        if (irq == 0x1) {
            log("receive timer interrupt");
            // clear timer interrupt
            w_sip(r_sip() & ~2);
        }
        else {
            PANIC(interrupt);
        }
    } else {
        // exception
        
    }
}


void
trap_init_hart()
{
    w_stvec((uint64)kernelvec);
    log("trap init hart");
}