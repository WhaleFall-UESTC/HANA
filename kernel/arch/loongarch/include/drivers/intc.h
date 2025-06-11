#ifndef __INTC_H__
#define __INTC_H__

#include <drivers/extioi.h>

#define UART0_IRQ 2

#define PIC_IRQ_BASE 0x00

#define MSI_IRQ_BASE 0x20
#define MSI_IRQ_VECS 0xe0

#define __irq_init(hart) \
    do { \
        extioi_init(hart); \
        ls7a_intc_init(); \
    } while(0)
#define __irq_init_default() __irq_init(DEFAULT_HART)
#define __irq_enable_default(irq) \
    do { \
        extioi_enable_irq(DEFAULT_HART, irq); \
        ls7a_enable_irq(irq); \
    } while(0)
#define __irq_disable_default(irq) \
    do { \
        ls7a_disable_irq(irq); \
        extioi_disable_irq(DEFAULT_HART, irq); \
    } while(0)
#define __irq_get() extioi_claim(DEFAULT_HART)
#define __irq_put(irq) \
    do { \
        extioi_complete(DEFAULT_HART, irq); \
        ls7a_intc_complete(irq); \
    } while(0)

void ls7a_intc_init(void);
void ls7a_intc_complete(uint64 irq);
void ls7a_enable_irq(int irq);
void ls7a_disable_irq(int irq);

#endif // __INTC_H__