#include <io/virtio.h>
#include <io/blk.h>
#include <drivers/virtio.h>
#include <common.h>
#include <debug.h>
#include <klib.h>
#include <riscv.h>
#include <mm/mm.h>
#define VIRTIO_BLK_DEV_NAME "vblk1"

void test_virtio() {
    char* buffer;
    struct blkdev* blkdev;
    struct blkreq* req;
    void* chan;

    // intr_on();
    
    buffer = (char *)kalloc(1024);
    assert(buffer != NULL);

    blkdev = blkdev_get_by_name(VIRTIO_BLK_DEV_NAME);
    assert(blkdev != NULL);
    log("blkdev %s found", blkdev->name);

    req = blkdev->ops->alloc(blkdev);
    assert(req != NULL);

    req->type = BLKREQ_TYPE_WRITE;
    req->sector_sta = 0;
    req->size = 1024;
    memset(buffer, 0x02, 1024);
    req->buffer = buffer;

    assert(blkdev->ops->submit != NULL);
    
    blkdev->ops->submit(blkdev, req);

    chan = blkreq_wait_channel(req);
    Log(ANSI_FG_YELLOW, "blkdev %s submit write request %p", blkdev->name, chan);
    
    blkdev_wait_all(blkdev);
    if (req->status == BLKREQ_STATUS_OK) {
        log("Write %ld bytes to sector %ld", req->size, req->sector_sta);
    } else {
        error("Failed to write to sector %ld", req->sector_sta);
    }
    blkdev_free_all(blkdev);
    
    // blkdev->ops->free(blkdev, req);
    
    memset(buffer, 0, 1024);
    
    req = blkdev->ops->alloc(blkdev);
    assert(req != NULL);
    
    req->type = BLKREQ_TYPE_READ;
    req->sector_sta = 0;
    req->size = 1024;
    req->buffer = buffer;
    
    blkdev->ops->submit(blkdev, req);
    
    chan = blkreq_wait_channel(req);
    Log(ANSI_FG_YELLOW, "blkdev %s submit read request %p", blkdev->name, chan);
    blkdev_wait_all(blkdev);
    if (req->status == BLKREQ_STATUS_OK)
    {
        log("Read %ld bytes from sector %ld", req->size, req->sector_sta);
    }
    else
    {
        error("Failed to read from sector %ld", req->sector_sta);
    }

    // blkdev->ops->free(blkdev, req);
    // blkdev_free_all(blkdev);

    assert(buffer[511] == 0x2);

    kfree(buffer);
}