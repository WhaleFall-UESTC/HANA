#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <common.h>
#include <locking/spinlock.h>
#include <tools/list.h>
#include <klib.h>
#include <irq/interrupt.h>

#define DEV_NAME_MAX_LEN 64

struct device {
    devid_t devid;                  // device number
    uint32 intr;                    // irq vec number

    char name[DEV_NAME_MAX_LEN];    // device name

    struct list_head dev_entry;     // entry in device list
    spinlock_t dev_lock;            // lock for accessing the device

    enum {
        DEVICE_TYPE_BLOCK,
        DEVICE_TYPE_CHAR,
        DEVICE_TYPE_OTHER,
    } type;                         // device type
};

/**
 * append a suffix to device name
 * @param name: device name buffer, must be large enough
 * @param buflen: buffer length of name
 * @param suffix: suffix to append
 */
void device_name_append_suffix(char *name, int buflen, const char *suffix);

/**
 * init device management system
 */
void device_subsys_init(void);

/**
 * initialize a device
 * @param device: pointer to device struct
 * @param devid: device id
 * @param intr: interrupt vector number
 * @param name: device name, must be unique
 */
void device_init(struct device* device, devid_t devid, uint32 intr, const char *name);

/**
 * register a device in list
 */
void device_register(struct device *device, irq_handler_t handler);

/**
 * get a device struct by its name
 */
struct device *device_get_by_name(const char *name);

/**
 * get a device struct by its device id
 */
struct device *device_get_by_id(devid_t id);

/**
 * We define some categories of device ids here
 * Devices allocation must be within its range.
 */
#define DEVID_VIRTIO_BLK_BASE 0x100
#define DEVID_VIRTIO_BLK_RANGE 0x3f
#define DEVID_VIRTIO_NET_BASE 0x140
#define DEVID_VIRTIO_NET_RANGE 0x3f
#define DEVID_UART 0x1

#endif // __DEVICE_H__