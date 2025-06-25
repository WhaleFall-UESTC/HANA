#include <io/net.h>
#include <mm/mm.h>
#include <debug.h>
#include <klib.h>

struct netdev *netdev_alloc(devid_t devid, int intr, const char *name, const struct netdev_ops *ops)
{
    KALLOC(struct netdev, dev);
    assert(dev != NULL);

    netdev_init(dev, devid, intr, name, ops);

    return dev;
}

void netdev_init(struct netdev *dev, devid_t devid, int intr, const char *name, const struct netdev_ops *ops)
{
    static char buffer[DEV_NAME_MAX_LEN];

    assert(dev != NULL);
    assert(name != NULL);
    assert(ops != NULL);

    snprintf(buffer, DEV_NAME_MAX_LEN, "%s%x", name, devid);
    device_init(&dev->dev, devid, intr, buffer);

    dev->ops = ops;
    dev->dev.type = DEVICE_TYPE_NET;
}

irqret_t netdev_general_isr(uint32 intid, void *private)
{
    struct netdev *netdev = (struct netdev *)private;
    irqret_t ret = IRQ_SKIP;

    assert(netdev != NULL);

    if (netdev->ops->irq_handle != NULL)
        ret = netdev->ops->irq_handle(netdev);
    else
    {
        log("netdev %s: no irq handler", netdev->dev.name);
        goto out;
    }

    if (ret == IRQ_ERR)
    {
        error("netdev %s: IRQ_ERR", netdev->dev.name);
        goto out;
    }
out:
    return ret;
}