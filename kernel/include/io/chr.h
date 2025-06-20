#ifndef __CHR_H__
#define __CHR_H__

#include <common.h>
#include <io/device.h>
#include <tools/list.h>
#include <locking/spinlock.h>
#include <irq/interrupt.h>

struct chrdev_ops;

struct chrdev
{
    struct device dev; // base device struct
    const struct chrdev_ops *ops;
    struct list_head chr_entry; // list entry for char devices
    spinlock_t chr_lock;
};

struct chrdev_ops
{
    void (*putchar)(struct chrdev *, char);
    char (*getchar)(struct chrdev *);
    irqret_t (*irq_handle)(struct chrdev *);
};

/**
 * init char device management system
 */
void char_subsys_init(void);

/**
 * alloc a char device and do initialization
 */
struct chrdev *chrdev_alloc(devid_t devid, int intr, const char *name, const struct chrdev_ops *ops);

/**
 * initialize a char device
 */
void chrdev_init(struct chrdev *dev, devid_t devid, int intr, const char *name, const struct chrdev_ops *ops);

/**
 * register char device in list
 * chardevs differ by devid(majo&minor)/name
 */
void chrdev_register(struct chrdev *chrdev);

/**
 * get a chrdev struct by its device name
 */
struct chrdev *chrdev_get_by_name(const char *name);

/**
 * get a chrdev struct by its device id
 */
struct chrdev *chrdev_get_by_id(devid_t id);

/**
 * get default or first chrdev struct of char device
 */
struct chrdev *chrdev_get_default_dev();

/**
 * general setup for char device irq response
 */
irqret_t chrdev_general_isr(uint32 intid, void * private);

#endif // __CHR_H__