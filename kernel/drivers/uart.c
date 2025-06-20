#include <drivers/uart.h>
#include <io/chr.h>
#include <fs/devfs/devfs.h>
#include <mm/mm.h>
#include <fs/fcntl.h>
#include <drivers/intc.h>

static void uart_putchar(struct chrdev *chrdev, char c) {
	putchar(c);
}

static char uart_getchar(struct chrdev *chrdev) {
	return getchar();
}

static irqret_t uart_irq_handle(struct chrdev *chrdev) {
	int ret;
	ret = uart_isr();

	if(ret < 0)
		return IRQ_ERR;
	return IRQ_HANDLED;
}

static const struct chrdev_ops uart_chrops = {
	.putchar = uart_putchar,
	.getchar = uart_getchar,
	.irq_handle = uart_irq_handle,
};

void uart_device_init(void) {
	struct chrdev* dev;

	dev = chrdev_alloc(DEVID_UART, UART0_IRQ, "uart", &uart_chrops);

	if(dev == NULL) {
		error("chrdev alloc failed.");
		return;
	}

	chrdev_register(dev);
}