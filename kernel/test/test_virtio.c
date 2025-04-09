#include <io/virtio.h>
#include <io/blk.h>
#include <drivers/virtio.h>
#include <common.h>
#include <debug.h>
#include <klib.h>
#include <riscv.h>
#define VIRTIO_BLK_DEV_NAME "vblk1"

void test_virtio() {
    char buffer[1024];
    struct blkdev* blkdev;
    struct blkreq* req;

    intr_on();

    blkdev = blkdev_get_by_name(VIRTIO_BLK_DEV_NAME);
    assert(blkdev != NULL);

    req = blkdev->ops->alloc(blkdev);
    assert(req != NULL);

    log("blkdev %s found\n", blkdev->name);

    req->type = BLKREQ_TYPE_READ;
    req->sector_sta = 0;
    req->size = 1024;
    memset(buffer, 0x02, sizeof(buffer));
    req->buffer = buffer;

    assert(blkdev->ops->submit != NULL);
    
    blkdev->ops->submit(blkdev, req);

    log("blkdev %s submit read request\n", blkdev->name);

    blkdev_wait_all(blkdev);
    if (req->status == BLKREQ_STATUS_OK) {
        log("Read %ld bytes from sector %ld\n", req->size, req->sector_sta);
    } else {
        error("Failed to read from sector %ld\n", req->sector_sta);
    }

    // blkdev->ops->free(blkdev, req);

    // memset(buffer, 0, sizeof(buffer));

    // req = blkdev->ops->alloc(blkdev);
    // assert(req != NULL);

    // req->type = BLKREQ_TYPE_WRITE;
    // req->sector_sta = 0;
    // req->size = 1024;
    // req->buffer = buffer;

    // blkdev->ops->submit(blkdev, req);

    // blkdev_wait_all(blkdev);
    // if (req->status == BLKREQ_STATUS_OK)
    // {
    //     log("Read %ld bytes from sector %ld\n", req->size, req->sector_sta);
    // }
    // else
    // {
    //     error("Failed to read from sector %ld\n", req->sector_sta);
    // }

    // blkdev->ops->free(blkdev, req);

    // assert(buffer[0] == 0x2);
}