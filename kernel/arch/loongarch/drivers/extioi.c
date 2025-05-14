#include <common.h>
#include <loongarch.h>
#include <debug.h>

void
extioi_init()
{
    // Enable uart0 interrupt
    iocsr_writeq(EXT_IOIen, 0x1UL << UART0_IRQ);

    // iocsr_writeq(EXT_IOIbounce, 0x1UL << UART0_IRQ);

    // 0-31 intr all mapped to HWI1
    iocsr_writeq(EXT_IOImap(0), 1);

    // node type1, route intr to core0 
    iocsr_writeq(EXT_IOImap_Core(UART0_IRQ), 0x01);

    iocsr_writeq(EXT_IOI_node_type(0), 1);
}


uint64
extioi_claim(int hart)
{
    return iocsr_readq(EXT_IOIsr(hart));
}

void
extioi_complete(int irq, int hart)
{
    iocsr_writeq(EXT_IOIsr(hart), irq);
}