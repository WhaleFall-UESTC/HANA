#include <common.h>
#include <loongarch.h>
#include <debug.h>
#include <mm/memlayout.h>

#define X(n, addr) *(volatile uint##n *)(addr)

void
apic_init()
{
    // allow uart0 interrupt
    X(64, LS7A_INT_MASK) = ~(0x1UL << UART0_IRQ);

    X(64, LS7A_INTEDGE) = (0x1UL << UART0_IRQ);

    X(8, LS7A1000_HTMSI_VECTOR0 + UART0_IRQ) = UART0_IRQ;

    X(64, LS7A_INT_POLARITY) = 0UL;
}

void 
apic_complete(uint64 irq)
{
    X(64, LS7A1000_INTCLR) = irq;
}