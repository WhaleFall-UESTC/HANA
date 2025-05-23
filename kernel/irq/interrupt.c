#include <common.h>

#ifdef ARCH_RISCV
#include <drivers/plic.h>
#elif defined(ARCH_LOONGARCH)
#include <loongarch.h>
#endif

#include <irq/interrupt.h>
#include <debug.h>
#include <trap/context.h>
#include <proc/proc.h>
#include <drivers/uart.h>

#ifdef ARCH_RISCV
static irq_handler_t irq_handlers[MAX_NR_IRQ];
static void* irq_privates[MAX_NR_IRQ];

int irq_register(unsigned int irq, irq_handler_t handler, void* dev) {
    if(irq >= MAX_NR_IRQ || irq_handlers[irq] != NULL) {
        return -1;
    }
    
    irq_handlers[irq] = handler;
    irq_privates[irq] = dev;

    __irq_enable_default(irq);

    return 0;
}

void irq_free(unsigned int irq) {
    __irq_disable_default(irq);

    irq_handlers[irq] = NULL;
    irq_privates[irq] = NULL;
}

void irq_response(void) {
    int irq, ret;

    irq = __irq_get();

    if(irq >= MAX_NR_IRQ || irq_handlers[irq] == NULL) {
        error("Irq %d too large or unregistered", irq);
        goto out;
    }

    ret = irq_handlers[irq](irq, irq_privates[irq]);

    if(ret == IRQ_ERR) {
        panic("Irq handle error");
    }

out:
    __irq_put(irq);
}

void irq_init(void) {
    __irq_init_default();
    w_sie(r_sie() | SIE_SSIE | SIE_SEIE);

    /**
     * TODO: move register uart irq to uart_init
     */
    #ifndef BIOS_SBI
    irq_register(UART0_IRQ, uart_isr, NULL);
    #endif
}
#endif

void irq_pushoff() {
    int old_intr_status = intr_get();
    intr_off();

    struct cpu *c = mycpu();
    if (c->noff == 0)
        c->intena = old_intr_status;
    c->noff++;
}

void irq_popoff() {
    Assert(!intr_get(), "interruptible");
    struct cpu *c = mycpu();
    assert(c->noff > 0);
    if (--c->noff == 0 && c->intena)
        intr_on();
}