#include <common.h>
#include <arch.h>
#include <platform.h>
#include <drivers/intc.h>

#define PLIC VIRT_PLIC
#define PLIC_ENABLE_S_MODE (PLIC + PLIC_SENABLE_OFFSET)
#define PLIC_THRESHOLD_S_MODE (PLIC + PLIC_STHRESHOLD_OFFSET)
#define PLIC_CLAIM_S_MODE (PLIC + PLIC_SCLAIM_OFFSET)
#define PLIC_PRIORITY (PLIC + PLIC_PRIORITY_OFFSET)

#define ENABLE_IRQ(hart, irq) \
    (*(volatile uint32 *)(PLIC_ENABLE_S_MODE + 4 * hart) |= (1 << irq))
#define DISABLE_IRQ(hart, irq) \
    (*(volatile uint32 *)(PLIC_ENABLE_S_MODE + 4 * hart) &= ~(1 << irq))
#define SET_IRQ_PRIORITY(irq, priority) \
    (*(volatile uint32 *)(PLIC_PRIORITY + 4 * irq) = priority)
#define SET_THRESHOLD(hart, threshold) \
    (*(volatile uint32 *)(PLIC_THRESHOLD_S_MODE + 4 * hart) = threshold)

void
plic_enable_irq(int hart, int irq, int priority)
{
    // enable this irq for the hart's S mode
    ENABLE_IRQ(hart, irq);
    // set this irq's priority
    SET_IRQ_PRIORITY(irq, priority);
}

void
plic_disable_irq(int hart, int irq) {
    DISABLE_IRQ(hart, irq);
    SET_IRQ_PRIORITY(irq, 0);
}

// get the claim id of the highest-priority pending interrupt for the current hart
int
plic_claim_irq(int hart)
{
    return *(volatile uint32 *)(PLIC_CLAIM_S_MODE + 4 * hart);
}

// tell the PLIC that we've handled this IRQ.
void
plic_complete_irq(int hart, int irq)
{
    *(volatile uint32 *)(PLIC_CLAIM_S_MODE + 4 * hart) = irq;
    // after this, the PLIC will clear the pending bit in the irq
    // and gateaway will "open" again
}
// P.S. Claim & Complete must appear in pairs, and should turn off interrupts


void
plic_init()
{
    SET_IRQ_PRIORITY(UART0_IRQ, 1);
    SET_IRQ_PRIORITY(VIRTIO0_IRQ, 1);
}

void
plic_init_hart(int hart)
{
    ENABLE_IRQ(hart, UART0_IRQ);
    ENABLE_IRQ(hart, VIRTIO0_IRQ);
    SET_THRESHOLD(hart, 0);
}

