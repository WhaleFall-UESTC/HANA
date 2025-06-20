#ifndef __IONET_H__
#define __IONET_H__

#include <common.h>
#include <io/device.h>
#include <irq/interrupt.h>
#include <net/net.h>
#include <debug.h>

struct netdev {
    struct device dev; // base device struct
    struct netif netif; // network interface this device is associated with
    const struct netdev_ops *ops;
};

struct netdev_ops {
    void (*send)(struct netdev *, struct packet *);
    void (*status)(struct netdev *);
    irqret_t (*irq_handle)(struct netdev *);
};

/**
 * alloc a net device and do initialization
 */
struct netdev *netdev_alloc(devid_t devid, int intr, const char *name, const struct netdev_ops *ops);

/**
 * initialize a net device
 */
void netdev_init(struct netdev *dev, devid_t devid, int intr, const char *name, const struct netdev_ops *ops);

/**
 * general setup for net device irq response
 */
irqret_t netdev_general_isr(uint32 intid, void * private);

/**
 * register net device in list
 * netdevs differ by devid/name
 */
static inline void netdev_register(struct netdev *netdev)
{
    assert(netdev != NULL);

    device_register(&netdev->dev, netdev_general_isr);
}

/**
 * get a netdev struct by its device name
 */
static inline struct netdev *netdev_get_by_name(const char *name) {
    return (struct netdev*)device_get_by_name(name, DEVICE_TYPE_NET);
}

/**
 * get a netdev struct by its device id
 */
static inline struct netdev *netdev_get_by_id(devid_t id) {
    return (struct netdev*)device_get_by_id(id, DEVICE_TYPE_NET);
}

/**
 * get default or first netdev struct of net device
 */
static inline struct netdev *netdev_get_default_dev() {
    return (struct netdev*)device_get_default(DEVICE_TYPE_NET);
}

#endif // __IONET_H__