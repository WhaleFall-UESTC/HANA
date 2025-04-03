#include <io/blk.h>
#include <mm/mm.h>
#include <klib.h>

struct blkdev *blkdev_alloc(int devid, unsigned long size,
                            const char *name, const struct blkdev_ops *ops)
{
    KALLOC(struct blkdev, dev);
    assert(dev != NULL);

    dev->devid = devid;
    dev->size = size;
    dev->ops = ops;

    strncpy(dev->name, name, BLKDEV_NAME_MAX_LEN);

    return dev;
}

