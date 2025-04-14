#include <io/blk.h>
#include <mm/mm.h>
#include <klib.h>
#include <irq/interrupt.h>
#include <locking/spinlock.h>
#include <proc/proc.h>

static struct list_head blkdev_list;
static spinlock_t blkdev_list_lock;

void blocks_init(void)
{
    INIT_LIST_HEAD(blkdev_list);
    spinlock_init(&blkdev_list_lock, "blkdev_list_lock");
}

struct blkdev *blkdev_alloc(int devid, unsigned long size, int intr,
                            const char *name, const struct blkdev_ops *ops)
{
    KALLOC(struct blkdev, dev);
    assert(dev != NULL);

    blkdev_init(dev, devid, intr, size, name, ops);

    return dev;
}

void blkdev_init(struct blkdev *dev, int devid, int intr, unsigned long size,
                   const char *name, const struct blkdev_ops *ops)
{
    assert(dev != NULL);
    assert(name != NULL);
    assert(ops != NULL);

    dev->devid = devid;
    dev->intr = intr;
    dev->size = size;
    dev->ops = ops;

    snprintf(dev->name, BLKDEV_NAME_MAX_LEN, "%s%d", name, intr);

    INIT_LIST_HEAD(dev->blk_list);
    INIT_LIST_HEAD(dev->rq_list);
    spinlock_init(&dev->rq_list_lock, dev->name);
}

void blkdev_register(struct blkdev *blkdev)
{
    assert(blkdev != NULL);

    spinlock_acquire(&blkdev_list_lock);
    list_insert(&blkdev_list, &blkdev->blk_list);
    spinlock_release(&blkdev_list_lock);

    irq_register(blkdev->intr, blkdev_general_isr, (void *)blkdev);

    debug("blkdev %s registered", blkdev->name);
}

struct blkdev *blkdev_get_by_name(const char *name)
{
    struct blkdev *blkdev;

    spinlock_acquire(&blkdev_list_lock);
    list_for_each_entry(blkdev, &blkdev_list, blk_list)
    {
        if (strncmp(blkdev->name, name, BLKDEV_NAME_MAX_LEN) == 0)
        {
            spinlock_release(&blkdev_list_lock);
            return blkdev;
        }
    }
    spinlock_release(&blkdev_list_lock);

    return NULL;
}

void blkdev_submit_req(struct blkdev *dev, struct blkreq *request) {
    assert(dev != NULL);
    assert(request != NULL);

    spinlock_acquire(&dev->rq_list_lock);
    list_insert(&dev->rq_list, &request->rq_head);
    spinlock_release(&dev->rq_list_lock);

    dev->ops->submit(dev, request);
}

static void blkdev_wait_endio(struct blkreq *request)
{
    assert(request != NULL);
    wakeup(blkreq_wait_channel(request));
}

void blkdev_submit_req_wait(struct blkdev *dev, struct blkreq *request) {
    warn_on(request->endio != NULL, "using synchronous submit when endio is set");
    request->endio = blkdev_wait_endio;
    blkdev_submit_req(dev, request);
    sleep(blkreq_wait_channel(request));
}

void blkdev_wait_all(struct blkdev *dev)
{
    struct blkreq *request;
    
    spinlock_acquire(&dev->rq_list_lock);
    list_for_each_entry(request, &dev->rq_list, rq_head)
    {
        assert(request != NULL);
        if(request->status == BLKREQ_STATUS_INIT)
        {
            sleep(blkreq_wait_channel(request));
        }
        else if(request->status == BLKREQ_STATUS_OK)
        {
            debug("Request completed successfully, sector=%ld, size=%ld, in device %s",
                request->sector_sta, request->size, dev->name);
        }
        else
        {
            error("Request failed, sector=%ld, size=%ld, in device %s",
                  request->sector_sta, request->size, dev->name);
        }
    }
    spinlock_release(&dev->rq_list_lock);
}

void blkdev_free_all(struct blkdev *dev) {
    struct blkreq *request, *tmp;

    spinlock_acquire(&dev->rq_list_lock);
    list_for_each_entry_safe(request, tmp, &dev->rq_list, rq_head)
    {
        // log("Freeing request %p in device %s", request, dev->name);
        dev->ops->free(dev, request);
        list_remove(&request->rq_head);
    }
    spinlock_release(&dev->rq_list_lock);
}

irqret_t blkdev_general_isr(uint32 intid, void *private) {
    struct blkdev *blkdev = (struct blkdev *)private;
    irqret_t ret = IRQ_SKIP;

    assert(blkdev != NULL);

    if (blkdev->ops->irq_handle != NULL)
        ret = blkdev->ops->irq_handle(blkdev);
    else {
        error("blkdev %s: no irq_handle", blkdev->name);
        goto out;
    }

    if (ret == IRQ_ERR) {
        error("blkdev %s: IRQ_ERR", blkdev->name);
        goto out;
    }
out:
    return ret;
}