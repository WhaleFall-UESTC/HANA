#include <io/chr.h>
#include <mm/mm.h>
#include <debug.h>
#include <klib.h>

static struct list_head chrdev_list;
static spinlock_t chrdev_list_lock;

void char_subsys_init(void)
{
    INIT_LIST_HEAD(chrdev_list);
    spinlock_init(&chrdev_list_lock, "chrdev_list_lock");
}

struct chrdev *chrdev_alloc(dev_t devid, int intr, const char *name, const struct chrdev_ops *ops)
{
    KALLOC(struct chrdev, dev);
    assert(dev != NULL);

    chrdev_init(dev, devid, intr, name, ops);

    return dev;
}

void chrdev_init(struct chrdev *dev, dev_t devid, int intr, const char *name, const struct chrdev_ops *ops)
{
    assert(dev != NULL);
    assert(name != NULL);
    assert(ops != NULL);

    dev->devid = devid;
    dev->intr = intr;
    dev->ops = ops;

    snprintf(dev->name, CHRDEV_NAME_MAX_LEN, "%s%d", name, intr);
    spinlock_init(&dev->chr_lock, dev->name);
}

void chrdev_register(struct chrdev *chrdev)
{
    assert(chrdev != NULL);

    spinlock_acquire(&chrdev_list_lock);
    list_insert(&chrdev_list, &chrdev->chr_entry);
    spinlock_release(&chrdev_list_lock);

    irq_register(chrdev->intr, chrdev_general_isr, (void *)chrdev);

    debug("chrdev %s registered", chrdev->name);
}

struct chrdev *chrdev_get_by_name(const char *name)
{
    struct chrdev *chrdev;

    spinlock_acquire(&chrdev_list_lock);
    list_for_each_entry(chrdev, &chrdev_list, chr_entry)
    {
        if (strncmp(chrdev->name, name, CHRDEV_NAME_MAX_LEN) == 0)
        {
            spinlock_release(&chrdev_list_lock);
            return chrdev;
        }
    }
    spinlock_release(&chrdev_list_lock);

    return NULL;
}

struct chrdev *chrdev_get_by_id(dev_t id)
{
    struct chrdev *chrdev;

    spinlock_acquire(&chrdev_list_lock);
    list_for_each_entry(chrdev, &chrdev_list, chr_entry)
    {
        if (chrdev->devid == id)
        {
            spinlock_release(&chrdev_list_lock);
            return chrdev;
        }
    }
    spinlock_release(&chrdev_list_lock);

    return NULL;
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
        log("chrdev %s: no irq handler", chrdev->name);
        goto out;
    }

    if (ret == IRQ_ERR)
    {
        error("chrdev %s: IRQ_ERR", chrdev->name);
        goto out;
    }
out:
    return ret;
}