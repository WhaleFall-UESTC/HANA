#include <io/device.h>
#include <io/chr.h>
#include <io/blk.h>

SPINLOCK_DEFINE(devlst_lock);
DECLARE_LIST_HEAD(device_list_head);

void device_init(struct device* device, devid_t devid, uint32 intr, const char *name) {
    static char buffer[SPINLOCK_NAME_MAX_LEN];
    static const char *locksuf = "-devlock";

    device->devid = devid;
    device->intr = intr;

    strncpy(device->name, name, DEV_NAME_MAX_LEN);

    strncpy(buffer, name, SPINLOCK_NAME_MAX_LEN);
    name_append_suffix(buffer, DEV_NAME_MAX_LEN, locksuf);
    spinlock_init(&device->dev_lock, buffer);
}

void device_register(struct device *device, irq_handler_t handler) {
    assert(device != NULL);

    spinlock_acquire(&devlst_lock);
    list_insert_end(&device_list_head, &device->dev_entry);
    spinlock_release(&devlst_lock);

    irq_register(device->intr, handler, (void *)device);

    debug("device %s registered", device->name);
}

struct device *device_get_by_name(const char *name, int type) {
    struct device *device;

    spinlock_acquire(&devlst_lock);
    list_for_each_entry(device, &device_list_head, dev_entry) {
        if ((device->type == type || device->type == DEVICE_TYPE_ANY) && strncmp(device->name, name, DEV_NAME_MAX_LEN) == 0) {
            spinlock_release(&devlst_lock);
            return device;
        }
    }
    spinlock_release(&devlst_lock);

    return NULL;
}

struct device *device_get_by_id(devid_t id, int type) {
    struct device *device;

    spinlock_acquire(&devlst_lock);
    list_for_each_entry(device, &device_list_head, dev_entry) {
        if ((device->type == type || device->type == DEVICE_TYPE_ANY) && device->devid == id) {
            spinlock_release(&devlst_lock);
            return device;
        }
    }
    spinlock_release(&devlst_lock);

    return NULL;
}

struct device *device_get_default(int type) {
    struct device *device;

    spinlock_acquire(&devlst_lock);
    list_for_each_entry(device, &device_list_head, dev_entry) {
        if (device->type == type || device->type == DEVICE_TYPE_ANY) {
            spinlock_release(&devlst_lock);
            return device;
        }
    }
    spinlock_release(&devlst_lock);

    return NULL;
}