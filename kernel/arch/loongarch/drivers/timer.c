#include <common.h>
#include <arch.h>
#include <debug.h>
#include <irq/interrupt.h>
#include <trap/context.h>
#include <proc/proc.h>
#include <proc/sched.h>
#include <time.h>

void timer_init() {
    // set timer id
    uint64 id = r_csr_cpuid();
    w_csr_tid(id);

    w_csr_tcfg(CSR_TCFG_Periodic | INTERVAL);
}

void timer_enable() {
    w_csr_tcfg(r_csr_tcfg() | CSR_TCFG_En);
}

void timer_isr() {
    // log("recieve timer interrupt");
    w_csr_ticlr(CSR_TICLR_CLR);
    yield();
}
