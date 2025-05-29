#include <common.h>
#include <arch.h>
#include <debug.h>
#include <drivers/intc.h>

#define ioscr_set_bit(base, cnt) \
    iocsr_writeq((base) + ((cnt) >> 3UL), \
        iocsr_readq((base) + ((cnt) >> 3UL)) | (0x1UL << ((cnt) & 0x7UL)));
#define ioscr_reset_bit(base, cnt) \
    iocsr_writeq((base) + ((cnt) >> 3UL), \
        iocsr_readq((base) + ((cnt) >> 3UL)) & ~(0x1UL << ((cnt) & 0x7UL)));

static inline uint64 lowbit(uint64 val) {
    return val & (-val);
}

static inline uint64 _log2(uint64 val) {
    uint64 res = 0;
    while(val >>= 1) res ++;
    return res;
}

void
extioi_init(int hart)
{
    /* extioi[31:0] map to cpu irq pin INT1, other to INT0 */
    iocsr_writeq(EXT_IOImap_Base, 1);

    /* nodetype0 set to 1, always trigger at node 0 */
    iocsr_writeq(EXT_IOI_node_type_Base, 1);
}


void extioi_enable_irq(int hart, int irq)
{
    // iocsr_writeq(EXT_IOIbounce, 0x1UL << UART0_IRQ);
    /* Enable interrupt */
    ioscr_set_bit(EXT_IOIen, irq);
    /* extioi route to core [hart] */
    iocsr_writeq(EXT_IOImap_Core_Base + irq, 0x1UL << hart);
}

void extioi_disable_irq(int hart, int irq)
{
    iocsr_writeq(EXT_IOImap_Core_Base + irq, 0);
    ioscr_reset_bit(EXT_IOIen, irq);
}

uint64
extioi_claim(int hart)
{
    return _log2(lowbit(iocsr_readq(EXT_IOIsr(hart))));
}

void
extioi_complete(int hart, int irq)
{
    ioscr_reset_bit(EXT_IOIsr(hart), irq);
}