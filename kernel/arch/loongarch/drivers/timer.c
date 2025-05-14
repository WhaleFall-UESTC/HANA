#include <common.h>
#include <loongarch.h>
#include <debug.h>

#define INTERVAL 0x1000000UL

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
    log("recieve timer interrupt");
    w_csr_ticlr(CSR_TICLR_CLR);
}
