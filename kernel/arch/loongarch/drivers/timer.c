#include <common.h>
#include <arch.h>
#include <debug.h>
#include <irq/interrupt.h>
#include <trap/context.h>
#include <proc/proc.h>
#include <proc/sched.h>
#include <time.h>

extern int timer_intr_get();

void timer_init() {
    // set timer id
    uint64 id = r_csr_cpuid();
    w_csr_tid(id);

    w_csr_tcfg(CSR_TCFG_Periodic | INTERVAL);
}

void timer_enable() {
    w_csr_tcfg(r_csr_tcfg() | CSR_TCFG_En);
}

static void account_time(struct proc* p) {
    if (r_csr_prmd() & CSR_PRMD_PPLV) {
        p->stime += 1;
    } else {
        p->utime += 1;
    }
}

void timer_isr() {
    // log("receive timer interrupt");
    w_csr_ticlr(CSR_TICLR_CLR);

    tick_counter += 1;
    struct proc* p = myproc();
    if (p && p->state == RUNNING) {
        account_time(p);
    }

    if (timer_intr_get())
        yield();
}
