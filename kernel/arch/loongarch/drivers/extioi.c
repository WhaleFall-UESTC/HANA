#include <common.h>
#include <arch.h>
#include <debug.h>
#include <drivers/intc.h>

/**
 * Set bit num.cnt from base
 * Aligned for 64bit
 */
#define ioscr_set_bit(base, cnt) \
    iocsr_writeq((base) + (((cnt) >> 6UL) << 3UL), \
    iocsr_readq((base) + (((cnt) >> 6UL) << 3UL)) | (0x1UL << ((cnt) & 0x3FUL)));
    // debug("setbit 0x%x in 0x%x", cnt, base);
    // debug("setbit write 0x%x for 0x%lx", ((cnt) >> 6UL) << 3UL, 0x1UL << ((cnt) & 0x3FUL));
#define ioscr_reset_bit(base, cnt) \
    iocsr_writeq((base) + (((cnt) >> 6UL) << 3UL), \
    iocsr_readq((base) + (((cnt) >> 6UL) << 3UL)) & ~(0x1UL << ((cnt) & 0x3FUL)));
// debug("resetbit 0x%x in 0x%x", cnt, base);
// debug("resetbit write 0x%x for 0x%lx", ((cnt) >> 6UL) << 3UL, 0x1UL << ((cnt) & 0x3FUL)); 

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
    // iocsr_writeq(EXT_IOIbounce0, 0x1UL << irq);
    /* Enable interrupt */
    debug("enable irq in hart%%%d, irq %d", hart, irq);
    ioscr_set_bit(EXT_IOIen, irq);
    /* extioi route to core [hart] */
    // iocsr_writeq(EXT_IOImap_Core_Base + (irq & ~0x07UL), 0);
    ioscr_set_bit(EXT_IOImap_Core_Base, (irq << 3) + hart);
}

void extioi_disable_irq(int hart, int irq)
{
    ioscr_reset_bit(EXT_IOImap_Core_Base, (irq << 3) + hart);
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
    ioscr_set_bit(EXT_IOIsr(hart), irq);
}