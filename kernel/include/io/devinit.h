#ifndef __DEVINIT_H__
#define __DEVINIT_H__

/**
 * Virtio init
 */
void virtio_device_init(void);

/**
 * Uart init
 */
struct chrdev* uart_device_init(void);

#endif // __DEVINIT_H__