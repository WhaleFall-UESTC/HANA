#include <io/blk.h>
#include <mm/mm.h>
#include <klib.h>
#include <irq/interrupt.h>
#include <locking/spinlock.h>
#include <proc/proc.h>

struct blkdev *blkdev_alloc(devid_t devid, unsigned long size, uint64 sector_size, int intr, const char *name, const struct blkdev_ops *ops)
{
    KALLOC(struct blkdev, dev);
    assert(dev != NULL);

    blkdev_init(dev, devid, size, sector_size, intr, name, ops);

    return dev;
}

void blkdev_init(struct blkdev *dev, devid_t devid, unsigned long size, uint64 sector_size, int intr, const char *name, const struct blkdev_ops *ops)
{
    static char buffer[DEV_NAME_MAX_LEN];

    assert(dev != NULL);
    assert(name != NULL);
    assert(ops != NULL);

    snprintf(buffer, DEV_NAME_MAX_LEN, "%s%x", name, devid);
    device_init(&dev->dev, devid, intr, buffer);

    dev->size = size;
    dev->sector_size = sector_size;
    dev->ops = ops;
    dev->dev.type = DEVICE_TYPE_BLOCK;

    device_name_append_suffix(buffer, SPINLOCK_NAME_MAX_LEN, "-blkrqlock");
    spinlock_init(&dev->rq_list_lock, buffer);
    INIT_LIST_HEAD(dev->rq_list);
}

void blkdev_submit_req(struct blkdev *dev, struct blkreq *request) {
    assert(dev != NULL);
    assert(request != NULL);

    spinlock_acquire(&dev->rq_list_lock);
    list_insert(&dev->rq_list, &request->rq_head);
    spinlock_release(&dev->rq_list_lock);

    // debug("doing request submit");
    dev->ops->submit(dev, request);
    // debug("request 0x%p submitted", request);
    // debug("%s", intr_get() ? "intr on" : "intr off");
}

void blkdev_submit_req_wait(struct blkdev *dev, struct blkreq *request) {
    blkdev_submit_req(dev, request);
    sleep(blkreq_wait_channel(request));
}

void blkdev_general_endio(struct blkreq *request)
{
    assert(request != NULL);
    if(request->endio != NULL)
        request->endio(request);
    // debug("request 0x%p finished and is ready to wakeup", request);
    blkreq_wakeup(request);
}

int blkdev_wait_all(struct blkdev *dev)
{
    struct blkreq *request;
    int ret = 0;
    
    spinlock_acquire(&dev->rq_list_lock);
    list_for_each_entry(request, &dev->rq_list, rq_head)
    {
        assert(request != NULL);
        if(request->status == BLKREQ_STATUS_INIT)
        {
            blkreq_sleep(request);
        }

        if(request->status == BLKREQ_STATUS_OK)
        {
            // debug("Request completed successfully, sector=%ld, size=%ld, in device %s",
            //     request->sector_sta, request->size, dev->dev.name);
        }
        else
        {
            error("Request failed, sector=%ld, size=%ld, in device %s",
                  request->sector_sta, request->size, dev->dev.name);
            ret ++;
        }
    }
    spinlock_release(&dev->rq_list_lock);
    return ret;
}

void blkdev_free_all(struct blkdev *dev) {
    struct blkreq *request, *tmp;

    spinlock_acquire(&dev->rq_list_lock);
    list_for_each_entry_safe(request, tmp, &dev->rq_list, rq_head)
    {
        if(request->status != BLKREQ_STATUS_OK)
        {
            error("Request not completed, sector=%ld, size=%ld, in device %s",
                  request->sector_sta, request->size, dev->dev.name);
            continue;
        }
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
        log("blkdev %s: no irq handler", blkdev->dev.name);
        goto out;
    }

    if (ret == IRQ_ERR) {
        error("blkdev %s: IRQ_ERR", blkdev->dev.name);
        goto out;
    }
out:
    return ret;
}