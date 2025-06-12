#include <drivers/intc.h>
#include <arch.h>
#include <mm/memlayout.h>
#include <common.h>
#include <debug.h>

void
ls7a_intc_init(void)
{
	ADDRVAL(64, LS7A_INT_POLARITY) = 0UL;
}

void 
ls7a_intc_complete(uint64 irq)
{
	assert(irq < 64);
	/**
	 * 电平触发不需要写边沿触发中断清除寄存器
	 */
	if(ADDRVAL(64, LS7A_INTEDGE) & (0x1UL << irq))
		ADDRVAL(64, LS7A_INTCLR) = irq;
}

void ls7a_enable_irq(int irq)
{
	assert(irq < 64);

	/* 启用HT消息中断 */
	ADDRVAL(64, LS7A_HTMSI_EN) |= (0x1UL << irq);

    /* 解除对应中断的屏蔽 (1<<irq对应的bit置0) */
    ADDRVAL(64, LS7A_INT_MASK) &= ~(0x1UL << irq);
    
	/**
	 * 实际为电平触发，不应该配置此项
	 * 详见 7A1000 手册 5.3 节
	 * 实际上在 qemu 中 UART 设置这一位不会生效
	 */
	// ADDRVAL(64, LS7A_INTEDGE) = (0x1UL << irq);

	/* 配置中断路由 */
    // ADDRVAL(8, LS7A_ROUTE_ENTRY0 + irq) = irq; // 路由到相同中断号

    /* 配置HT中断向量路由 */
    ADDRVAL(8, LS7A_HTMSI_VECTOR0 + irq) = irq; // 路由到相同中断号
}

void ls7a_disable_irq(int irq)
{
	assert(irq < 64);

    /* 屏蔽对应中断 (1<<irq对应的bit置1) */
    ADDRVAL(64, LS7A_INT_MASK) |= (0x1UL << irq);
}