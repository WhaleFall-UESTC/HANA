#ifndef __CHR_H__
#define __CHR_H__

#include <common.h>
#include <tools/list.h>
#include <locking/spinlock.h>
#include <irq/interrupt.h>

#define CHRDEV_NAME_MAX_LEN 64

struct chrdev_ops;

struct chrdev
{
    devid_t devid;
    uint32 intr;
    char name[CHRDEV_NAME_MAX_LEN];
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

#define UART_CHRDEV_DEVID 0

#endif // __CHR_H__