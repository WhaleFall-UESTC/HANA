#ifndef __CHR_H__
#define __CHR_H__

#include <common.h>
#include <io/device.h>
#include <irq/interrupt.h>
#include <debug.h>

struct chrdev_ops;

struct chrdev
{
    struct device dev; // base device struct
    const struct chrdev_ops *ops;
};

struct chrdev_ops
{
    void (*putchar)(struct chrdev *, char);
    char (*getchar)(struct chrdev *);
    irqret_t (*irq_handle)(struct chrdev *);
};

/**
 * alloc a char device and do initialization
 */
struct chrdev *chrdev_alloc(devid_t devid, int intr, const char *name, const struct chrdev_ops *ops);

/**
 * initialize a char device
 */
void chrdev_init(struct chrdev *dev, devid_t devid, int intr, const char *name, const struct chrdev_ops *ops);

/**
 * general setup for char device irq response
 */
irqret_t chrdev_general_isr(uint32 intid, void * private);

/**
 * register char device in list
 * chardevs differ by devid/name
 */
static inline void chrdev_register(struct chrdev *chrdev)
{
    assert(chrdev != NULL);

    device_register(&chrdev->dev, chrdev_general_isr);
}

/**
 * get a chrdev struct by its device name
 */
static inline struct chrdev *chrdev_get_by_name(const char *name) {
    return (struct chrdev*)device_get_by_name(name, DEVICE_TYPE_CHAR);
}

/**
 * get a chrdev struct by its device id
 */
static inline struct chrdev *chrdev_get_by_id(devid_t id) {
    return (struct chrdev*)device_get_by_id(id, DEVICE_TYPE_CHAR);
}

/**
 * get default or first chrdev struct of char device
 */
static inline struct chrdev *chrdev_get_default_dev() {
    return (struct chrdev*)device_get_default(DEVICE_TYPE_CHAR);
}

#endif // __CHR_H__