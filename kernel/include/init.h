#ifndef __INIT_H__
#define __INIT_H__

/**
 * Initialize all virtio devices present in the system.
 */
void virtio_device_init(void);

/**
 * Uart init
 */
struct chrdev* uart_device_init(void);

/**
 * Init after kernel init
 */
void device_init2(void);

#endif // __INIT_H__