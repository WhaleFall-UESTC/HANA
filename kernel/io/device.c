#include <io/device.h>
#include <io/chr.h>
#include <io/blk.h>

struct list_head device_list_head;
spinlock_t devlst_lock;

void device_name_append_suffix(char *name, int buflen, const char *suffix) {
    int len = strlen(name), suflen = strlen(suffix);
    int pos;

    if(len + suflen < buflen) {
        pos = len;
    }
    else if(buflen < len) {
        pos = buflen - suflen - 1;
    }
    else {
        error("device name buffer too small for suffix");
        return;
    }

    assert(pos >= 0 && pos + suflen < buflen);
    strncpy(name + pos, suffix, suflen);
    name[pos + suflen] = '\0';
}

void device_subsys_init(void) {
    INIT_LIST_HEAD(device_list_head);
    spinlock_init(&devlst_lock, "device list lock");

    block_subsys_init();
    char_subsys_init();
}

void device_init(struct device* device, devid_t devid, uint32 intr, const char *name) {
    static char buffer[SPINLOCK_NAME_MAX_LEN];
    static const char *locksuf = "-devlock";

    device->devid = devid;
    device->intr = intr;

    strncpy(device->name, name, DEV_NAME_MAX_LEN);

    strncpy(buffer, name, SPINLOCK_NAME_MAX_LEN);
    device_name_append_suffix(buffer, DEV_NAME_MAX_LEN, locksuf);
    spinlock_init(&device->dev_lock, buffer);
}

void device_register(struct device *device, irq_handler_t handler) {
    assert(device != NULL);

    spinlock_acquire(&devlst_lock);
    list_insert(&device_list_head, &device->dev_entry);
    spinlock_release(&devlst_lock);

    irq_register(device->intr, handler, (void *)device);

    debug("device %s registered", device->name);
}

struct device *device_get_by_name(const char *name) {
    struct device *device;

    spinlock_acquire(&devlst_lock);
    list_for_each_entry(device, &device_list_head, dev_entry) {
        if (strncmp(device->name, name, DEV_NAME_MAX_LEN) == 0) {
            spinlock_release(&devlst_lock);
            return device;
        }
    }
    spinlock_release(&devlst_lock);

    return NULL;
}

struct device *device_get_by_id(devid_t id) {
    struct device *device;

    spinlock_acquire(&devlst_lock);
    list_for_each_entry(device, &device_list_head, dev_entry) {
        if (device->devid == id) {
            spinlock_release(&devlst_lock);
            return device;
        }
    }
    spinlock_release(&devlst_lock);

    return NULL;
}

