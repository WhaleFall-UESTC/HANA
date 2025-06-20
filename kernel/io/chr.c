#include <io/chr.h>
#include <mm/mm.h>
#include <debug.h>
#include <klib.h>

struct chrdev *chrdev_alloc(devid_t devid, int intr, const char *name, const struct chrdev_ops *ops)
{
    KALLOC(struct chrdev, dev);
    assert(dev != NULL);

    chrdev_init(dev, devid, intr, name, ops);

    return dev;
}

void chrdev_init(struct chrdev *dev, devid_t devid, int intr, const char *name, const struct chrdev_ops *ops)
{
    static char buffer[DEV_NAME_MAX_LEN];

    assert(dev != NULL);
    assert(name != NULL);
    assert(ops != NULL);

    snprintf(buffer, DEV_NAME_MAX_LEN, "%s%x", name, devid);
    device_init(&dev->dev, devid, intr, buffer);

    dev->ops = ops;
    dev->dev.type = DEVICE_TYPE_CHAR;
}

irqret_t chrdev_general_isr(uint32 intid, void *private)
{
    struct chrdev *chrdev = (struct chrdev *)private;
    irqret_t ret = IRQ_SKIP;

    assert(chrdev != NULL);

    if (chrdev->ops->irq_handle != NULL)
        ret = chrdev->ops->irq_handle(chrdev);
    else
    {
        log("chrdev %s: no irq handler", chrdev->dev.name);
        goto out;
    }

    if (ret == IRQ_ERR)
    {
        error("chrdev %s: IRQ_ERR", chrdev->dev.name);
        goto out;
    }
out:
    return ret;
}