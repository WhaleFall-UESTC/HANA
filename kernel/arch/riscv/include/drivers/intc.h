#ifndef __INTC_H__
#define __INTC_H__

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

#define __irq_init(hart) \
    do { \
        plic_init(); \
        plic_init_hart(hart); \
    } while(0)
#define __irq_init_default() \
    do { \
        __irq_init(DEFAULT_HART); \
        w_sie(r_sie() | SIE_SSIE | SIE_SEIE); \
    } while(0)
#define __irq_enable_default(irq) \
    plic_enable_irq(DEFAULT_HART, irq, DEFAULT_PRI)
#define __irq_disable_default(irq) \
    plic_disable_irq(DEFAULT_HART, irq)
#define __irq_get() plic_claim_irq(DEFAULT_HART)
#define __irq_put(irq) plic_complete_irq(DEFAULT_HART, irq)

/**
 * PLIC interrupt enable an interrupt
 * @param hart Hart ID
 * @param irq Interrupt number
 * @param priority Priority of the interrupt
 */
void plic_enable_irq(int hart, int irq, int priority);

/**
 * Disable a PLIC interrupt
 * @param hart Hart ID
 * @param irq Interrupt number
 */
void plic_disable_irq(int hart, int irq);

/**
 * Claim a PLIC interrupt
 * @param hart Hart ID
 * @return The claimed interrupt number, or 0 if no interrupt is available
 */
int plic_claim_irq(int hart);

/**
 * Complete a PLIC interrupt
 * @param hart Hart ID
 * @param irq Interrupt number
 */
void plic_complete_irq(int hart, int irq);

/**
 * Initialize the PLIC (Platform-Level Interrupt Controller)
 */
void plic_init();

/**
 * Initialize the PLIC for a specific hart
 * @param hart Hart ID
 */
void plic_init_hart(int hart);

#endif // __INTC_H__