#include <drivers/intc.h>
#include <arch.h>
#include <mm/memlayout.h>
#include <common.h>

void
ls7a_intc_init(void)
{
	// /* 启用HT消息中断 */
	// ADDRVAL(64, LS7A_HTMSI_EN) |= (0x1UL << UART0_IRQ);

	/* 解除中断屏蔽 */
	// ADDRVAL(64, LS7A_INT_MASK) &= ~(0x1UL << UART0_IRQ);

	/**
	 * 实际为电平触发，不应该配置此项
	 * 详见 7A1000 手册 5.3 节
	 */
	// ADDRVAL(64, LS7A_INTEDGE) = (0x1UL << UART0_IRQ);

	/* route to the same irq in extioi */
	// ADDRVAL(8, LS7A1000_HTMSI_VECTOR0 + UART0_IRQ) = UART0_IRQ;

	ADDRVAL(64, LS7A_INT_POLARITY) = 0UL;
}

void 
ls7a_intc_complete(uint64 irq)
{
	/**
	 * 电平触发不需要写边沿触发中断清除寄存器
	 */
	// ADDRVAL(64, LS7A1000_INTCLR) = irq;
}

void ls7a_enable_irq(int irq)
{
    /* 解除对应中断的屏蔽 (1<<irq对应的bit置0) */
    ADDRVAL(64, LS7A_INT_MASK) &= ~(0x1UL << irq);
    
    /* 配置HT中断向量路由 */
    ADDRVAL(8, LS7A_HTMSI_VECTOR0 + irq) = irq; // 路由到相同中断号
}

void ls7a_disable_irq(int irq)
{
    /* 屏蔽对应中断 (1<<irq对应的bit置1) */
    ADDRVAL(64, LS7A_INT_MASK) |= (0x1UL << irq);
}