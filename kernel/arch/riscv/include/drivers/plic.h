#ifndef __PLIC_H__
#define __PLIC_H__

#define PLIC_PRIORITY_OFFSET    0x0
#define PLIC_PENDING_OFFSET     0x1000
#define PLIC_ENABLE_OFFSET      0x2000
#define PLIC_THRESHOLD_OFFSET   0x200000
#define PLIC_CLAIM_OFFSET       0x200004

#define PLIC_SENABLE_OFFSET     0x80 + PLIC_ENABLE_OFFSET
#define PLIC_STHRESHOLD_OFFSET  0x1000 + PLIC_THRESHOLD_OFFSET
#define PLIC_SCLAIM_OFFSET      0x1000 + PLIC_CLAIM_OFFSET

#define UART0_IRQ               10
#define VIRTIO0_IRQ             1

#define irq_init(hart) \
    do { \
        plic_init(); \
        plic_init_hart(hart); \
    } while(0);
#define irq_init_default() irq_init(DEFAULT_HART)
#define irq_enable_default(irq) \
    plic_enable_irq(DEFAULT_HART, irq, DEFAULT_PRI)
#define irq_disable_default(irq) \
    plic_disable_irq(DEFAULT_HART, irq)
#define irq_get() plic_claim_irq(DEFAULT_HART)
#define irq_put(irq) plic_complete_irq(DEFAULT_HART, irq)

void plic_enable_irq(int hart, int irq, int priority);
void plic_disable_irq(int hart, int irq);
int plic_claim_irq(int hart);
void plic_complete_irq(int hart, int irq);
void plic_init();
void plic_init_hart(int hart);

#endif // __PLIC_H__