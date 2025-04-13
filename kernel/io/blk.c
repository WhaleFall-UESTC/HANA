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

struct blkdev *blkdev_alloc(int devid, unsigned long size,   
                            const char *name, const struct blkdev_ops *ops)
{
    KALLOC(struct blkdev, dev);
    assert(dev != NULL);

    dev->devid = devid;
    dev->size = size;
    dev->ops = ops;

    strncpy(dev->name, name, BLKDEV_NAME_MAX_LEN);

    blkdev_init(dev);

    return dev;
}


void blkdev_init(struct blkdev* dev)
{
    INIT_LIST_HEAD(dev->blk_list);
    INIT_LIST_HEAD(dev->rq_list);
    spinlock_init(&dev->rq_list_lock, "blkdev rq_list lock");
}

void blkdev_register(struct blkdev *blkdev)
{
    assert(blkdev != NULL);

    spinlock_acquire(&blkdev_list_lock);
    list_insert(&blkdev_list, &blkdev->blk_list);
    spinlock_release(&blkdev_list_lock);

    log("blkdev %s registered", blkdev->name);
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
            log("Request completed successfully, sector=%ld, size=%ld, in device %s",
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
        dev->ops->free(dev, request);
        list_remove(&request->rq_head);
    }
    spinlock_release(&dev->rq_list_lock);
}