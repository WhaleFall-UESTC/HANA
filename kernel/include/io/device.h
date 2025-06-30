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

    enum devtype {
        DEVICE_TYPE_BLOCK,
        DEVICE_TYPE_CHAR,
        DEVICE_TYPE_NET,
        DEVICE_TYPE_OTHER,
        DEVICE_TYPE_ANY,
    } type;                         // device type
};

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
 * @param device: pointer to device struct
 * @param handler: interrupt handler function
 */
void device_register(struct device *device, irq_handler_t handler);

/**
 * get a device struct by its name
 * @param name: device name
 * @param type: device type, use DEVICE_TYPE_ANY to match any type
 */
struct device *device_get_by_name(const char *name, int type);

/**
 * get a device struct by its device id
 * @param id: device id
 * @param type: device type, use DEVICE_TYPE_ANY to match any type
 * @return: pointer to device struct, or NULL if not found
 */
struct device *device_get_by_id(devid_t id, int type);

/**
 * get default or first device struct of a specific type
 * @param type: device type, use DEVICE_TYPE_ANY to match any type
 * @return: pointer to device struct, or NULL if not found
 */
struct device *device_get_default(int type);

/**
 * We define some categories of device ids here
 * Devices allocation must be within its range.
 */
#define DEVID_VIRTIO_BLK_BASE 0x100
#define DEVID_VIRTIO_BLK_RANGE 0x3f
#define DEVID_VIRTIO_NET_BASE 0x140
#define DEVID_VIRTIO_NET_RANGE 0x3f
#define DEVID_UART 0x1

extern struct list_head device_list_head;
extern spinlock_t devlst_lock;

#define device_list_iter_next_locked(devptr) \
    ({ \
        void* __ret; \
        spinlock_acquire(&devlst_lock); \
        __ret = list_iter_next(devptr, &device_list_head, dev_entry); \
        spinlock_release(&devlst_lock); \
        __ret; \
    })

#define device_list_iter_init_locked(devptr) \
    ({ \
        void* __ret; \
        spinlock_acquire(&devlst_lock); \
        __ret = list_iter_init(devptr, &device_list_head, dev_entry); \
        spinlock_release(&devlst_lock); \
        __ret; \
    })

#define device_list_for_each_entry(devptr) \
    list_for_each_entry(devptr, &device_list_head, dev_entry)

#define device_list_for_each_entry_safe(devptr, nextptr) \
    list_for_each_entry(devptr, nextptr, &device_list_head, dev_entry)

/**
 * Iterate over the device list with a locked iterator.
 */
#define device_list_for_each_entry_locked(devptr) \
    for(device_list_iter_init_locked(devptr); \
        (devptr) != NULL; device_list_iter_next_locked(devptr))

#endif // __DEVICE_H__