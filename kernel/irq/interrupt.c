#include <common.h>
#include <interrupt.h>
#include <debug.h>

#ifdef ARCH_RISCV
#include <plic.h>
#else
/* other archs */
#endif

static irq_handler_t irq_handlers[MAX_NR_IRQ];
static void* irq_privates[MAX_NR_IRQ];

int register_irq(unsigned int irq, irq_handler_t handler, void* dev) {
    if(irq >= MAX_NR_IRQ || irq_handlers[irq] != NULL) {
        return -1;
    }
    
    irq_handlers[irq] = handler;
    irq_privates[irq] = dev;

    irq_enable_default(irq);

    return 0;
}

void free_irq(unsigned int irq) {
    irq_disable_default(irq);

    irq_handlers[irq] = NULL;
    irq_privates[irq] = NULL;
}

void response_interrupt(void) {
    int irq, ret;

    irq = irq_get();

    if(irq >= MAX_NR_IRQ || irq_handlers[irq] != NULL) {
        panic("Irq too large or unregistered");
        goto out;
    }

    ret = irq_handlers[irq](irq, irq_privates[irq]);

    if(ret == IRQ_ERR) {
        panic("Irq handle error");
    }

out:
    irq_put(irq);
}

void interrupt_init(void) {
    irq_init_default();
}