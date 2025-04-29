#include <drivers/uart.h>
#include <io/chr.h>
#include <fs/devfs/devfs.h>
#include <mm/mm.h>
#include <fs/fcntl.h>

#ifdef ARCH_RISCV
#include <drivers/plic.h>
#endif

static void uart_putchar(struct chrdev *chrdev, char c) {
	putchar(c);
}

static char uart_getchar(struct chrdev *chrdev) {
	return getchar();
}

static irqret_t uart_irq_handle(struct chrdev *chrdev) {
	int ret;

#ifndef BIOS_SBI
	ret = uart_isr();

	if(ret < 0)
		return IRQ_ERR;
#endif
	return IRQ_HANDLED;
}

static const struct chrdev_ops uart_chrops = {
	.putchar = uart_putchar,
	.getchar = uart_getchar,
	.irq_handle = uart_irq_handle,
};

void uart_device_init(void) {
	struct chrdev* dev;

	dev = chrdev_alloc(UART_CHRDEV_DEVID, UART0_IRQ, "uart", &uart_chrops);

	if(dev == NULL) {
		error("chrdev alloc failed.");
		return;
	}

	chrdev_register(dev);
}