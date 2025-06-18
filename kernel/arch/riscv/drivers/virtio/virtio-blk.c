/**
 * This code is partly copied from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/virtio-blk.c
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#include <common.h>
#include <debug.h>
#include <mm/mm.h>
#include <drivers/virtio-mmio.h>
// #include <locking/spinsem.h>
#include <locking/spinlock.h>
#include <irq/interrupt.h>
#include <proc/proc.h>
#include <io/blk.h>
#include <klib.h>
#include <arch.h>

#define VIRTIO_BLK_DEV_NAME "virtio-blk"

struct virtio_cap blk_caps[] = {
    {"VIRTIO_BLK_F_SIZE_MAX", 1, false,
     "Maximum size of any single segment is in size_max."},
    {"VIRTIO_BLK_F_SEG_MAX", 2, false,
     "Maximum number of segments in a request is in seg_max."},
    {"VIRTIO_BLK_F_GEOMETRY", 4, false,
     "Disk-style geometry specified in geometry."},
    {"VIRTIO_BLK_F_RO", 5, false, "Device is read-only."},
    {"VIRTIO_BLK_F_BLK_SIZE", 6, false,
     "Block size of disk is in blk_size."},
    {"VIRTIO_BLK_F_FLUSH", 9, false, "Cache flush command support."},
    {"VIRTIO_BLK_F_TOPOLOGY", 10, false,
     "Device exports information on optimal I/O alignment."},
    {"VIRTIO_BLK_F_CONFIG_WCE", 11, false,
     "Device can toggle its cache between writeback and "
     "writethrough modes."},
    VIRTIO_INDP_CAPS};

#define get_vblkreq(req) container_of(req, struct virtio_blk_req, blkreq)
#define get_vblkdev(dev) container_of(dev, struct virtio_blk, blkdev)

struct virtio_blk
{
    virtio_regs *regs;
    struct virtio_blk_config *config;
    struct virtq_info *virtq_info;
    uint32 intid;
    struct list_head list;
    struct blkdev blkdev;
};

static void virtio_blk_handle_used(struct virtio_blk *dev, uint32 usedidx)
{
    struct virtq_info* virtq_info = dev->virtq_info;
    volatile struct virtqueue *virtq = virtq_info->virtq;
    uint32 desc1, desc2, desc3;
    struct virtio_blk_req *req;

    // debug("virtio_blk_handle_used: usedidx=%u, usedid=%u",
    //       usedidx, virtq->used.ring[usedidx].id);

    desc1 = virtq->used.ring[usedidx].id;
    if (!(virtq->desc[desc1].flags & VIRTQ_DESC_F_NEXT))
        goto bad_desc;
    desc2 = virtq->desc[desc1].next;
    if (!(virtq->desc[desc2].flags & VIRTQ_DESC_F_NEXT))
        goto bad_desc;
    desc3 = virtq->desc[desc2].next;
    if (virtq->desc[desc1].len != VIRTIO_BLK_REQ_HEADER_SIZE ||
        // virtq->desc[desc2].len != VIRTIO_BLK_SECTOR_SIZE ||
        virtq->desc[desc3].len != VIRTIO_BLK_REQ_FOOTER_SIZE)
        goto bad_desc;

    req = dev->virtq_info->desc_virt[desc1];

    virtq_free_desc(virtq_info, desc1);
    virtq_free_desc(virtq_info, desc2);
    virtq_free_desc(virtq_info, desc3);

    switch (req->status)
    {
    case VIRTIO_BLK_S_OK:
        req->blkreq.status = BLKREQ_STATUS_OK;
        // debug("virtio_blk_handle_used: request completed successfully");
        break;
    case VIRTIO_BLK_S_IOERR:
        debug("virtio_blk_handle_used: request failed with I/O error");
        req->blkreq.status = BLKREQ_STATUS_ERR;
        break;
    default:
        panic("Unhandled status in virtio_blk irq");
    }

    blkdev_general_endio(&req->blkreq);

    return;
bad_desc:
    error("virtio-blk received malformed descriptors");
    return;
}

static irqret_t virtio_blk_isr(struct blkdev *blkdev)
{
    int i;
    struct virtio_blk *dev = get_vblkdev(blkdev);
    struct virtq_info* virtq_info = dev->virtq_info;

    if (!dev)
    {
        panic("virtio-blk: received IRQ for unknown device!");
        return IRQ_ERR;
    }
    
    for (i = virtq_info->seen_used; i != (virtq_info->virtq->used.idx % VIRTIO_DEFAULT_QUEUE_SIZE);
    i = wrap(i + 1, VIRTIO_DEFAULT_QUEUE_SIZE))
    {
        virtio_blk_handle_used(dev, i);
    }
    virtq_info->seen_used = virtq_info->virtq->used.idx % VIRTIO_DEFAULT_QUEUE_SIZE;
    
    WRITE32(dev->regs->InterruptACK, READ32(dev->regs->InterruptStatus));

    return IRQ_HANDLED;
}

static void virtio_blk_send(struct virtio_blk *blk, struct virtio_blk_req *hdr)
{
    volatile struct virtqueue *virtq = blk->virtq_info->virtq;
    virtq->avail.ring[virtq->avail.idx % VIRTIO_DEFAULT_QUEUE_SIZE] =
        hdr->descriptor;
    mb();
    virtq->avail.idx += 1;
    mb();
    WRITE32(blk->regs->QueueNotify, 0);
}

static void virtio_blk_status(struct blkdev *dev)
{
    struct virtio_blk *blkdev = get_vblkdev(dev);
    volatile struct virtqueue *virtq = blkdev->virtq_info->virtq;
    log("virtio_blk_dev at 0x%lx",
           virt_to_phys((uint64)blkdev->regs));
    log("    Status=0x%x", READ32(blkdev->regs->Status));
    log("    DeviceID=0x%x", READ32(blkdev->regs->DeviceID));
    log("    VendorID=0x%x", READ32(blkdev->regs->VendorID));
    log("    InterruptStatus=0x%x",
           READ32(blkdev->regs->InterruptStatus));
    log("    MagicValue=0x%x", READ32(blkdev->regs->MagicValue));
    log("  Queue 0:");
    log("    avail.idx = %u", virtq->avail.idx);
    log("    used.idx = %u", virtq->used.idx);
    WRITE32(blkdev->regs->QueueSel, 0);
    mb();
    virtq_show(blkdev->virtq_info);
}

static struct blkreq *virtio_blk_alloc(struct blkdev *dev)
{
    KALLOC(struct virtio_blk_req, vblkreq);
    blkreq_init(&vblkreq->blkreq, dev);
    return &vblkreq->blkreq;
}

static void virtio_blk_free(struct blkdev *dev, struct blkreq *req)
{
    struct virtio_blk_req *vblkreq = get_vblkreq(req);
    kfree(vblkreq);
}

static void virtio_blk_submit(struct blkdev *dev, struct blkreq *req)
{
    struct virtio_blk *blk = get_vblkdev(dev);
    struct virtio_blk_req *hdr = get_vblkreq(req);
    struct virtq_info* virtq_info = blk->virtq_info;
    volatile struct virtqueue *virtq = blk->virtq_info->virtq;
    uint32 d1, d2, d3, datamode = 0;

    if(req->size & (VIRTIO_BLK_SECTOR_SIZE - 1))
    {
        error("size not aligned to sector size");
        return;
    }

    if (req->type == BLKREQ_TYPE_READ)
    {
        hdr->type = VIRTIO_BLK_T_IN;
        datamode = VIRTQ_DESC_F_WRITE; /* mark page writeable */
    }
    else
    {
        hdr->type = VIRTIO_BLK_T_OUT;
    }
    hdr->sector = req->sector_sta;

    // debug("virtio_blk_submit: %s, sector=%lu, size=%lu",
    //       req->type == BLKREQ_TYPE_READ ? "read" : "write",
    //       req->sector_sta, req->size);

    d1 = virtq_alloc_desc(virtq_info, hdr);
    hdr->descriptor = d1;
    virtq->desc[d1].len = VIRTIO_BLK_REQ_HEADER_SIZE;
    virtq->desc[d1].flags = VIRTQ_DESC_F_NEXT;

    d2 = virtq_alloc_desc(virtq_info, req->buffer);
    // virtq->desc[d2].len = VIRTIO_BLK_SECTOR_SIZE;
    virtq->desc[d2].len = req->size;
    virtq->desc[d2].flags = datamode | VIRTQ_DESC_F_NEXT;

    d3 = virtq_alloc_desc(virtq_info,
                          (void *)hdr + VIRTIO_BLK_REQ_HEADER_SIZE);
    virtq->desc[d3].len = VIRTIO_BLK_REQ_FOOTER_SIZE;
    virtq->desc[d3].flags = VIRTQ_DESC_F_WRITE;

    virtq->desc[d1].next = d2;
    virtq->desc[d2].next = d3;
    virtq->desc[d3].next = 0;

    virtio_blk_send(blk, hdr);
}

struct blkdev_ops virtio_blk_ops = {
    .alloc = virtio_blk_alloc,
    .free = virtio_blk_free,
    .submit = virtio_blk_submit,
    .status = virtio_blk_status,
    .irq_handle = virtio_blk_isr,
};

int virtio_blk_init(volatile virtio_regs *regs, uint32 intid)
{
    struct virtio_blk *vdev;
    struct virtq_info *virtq_info;
    uint64 blk_size, _blk_size;

    vdev = kalloc(sizeof(struct virtio_blk));

    // Read and write feature bits
    virtio_check_capabilities(regs, blk_caps, nr_elem(blk_caps));

    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_FEATURES_OK);
	mb();
	if (!(regs->Status & VIRTIO_STATUS_FEATURES_OK)) {
		error("virtio-blk did not accept our features");
		return -1;
	}

    // Perform device-specific setup
    virtq_info = virtq_add_to_device(regs, 0);
    assert(virtq_info != NULL);

    vdev->regs = regs;
    vdev->virtq_info = virtq_info;
    vdev->intid = intid;
    vdev->config = (struct virtio_blk_config *)&regs->Config;

    // Read device configuration fields
    blk_size = READ64(vdev->config->capacity);

    do {
        _blk_size = blk_size;
        mb();
        blk_size = READ64(vdev->config->capacity);
    } while(blk_size != _blk_size);
    
    // Set DRIVER_OK status bit
    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_DRIVER_OK);
    mb();

    blkdev_init(&vdev->blkdev, intid, blk_size * VIRTIO_BLK_SECTOR_SIZE,
                VIRTIO_BLK_SECTOR_SIZE, intid, VIRTIO_BLK_DEV_NAME, &virtio_blk_ops);
    debug("virtio-blk: %s, size=%lu, intid=%d", vdev->blkdev.name, vdev->blkdev.size, vdev->intid);
    // debug("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    blkdev_register(&vdev->blkdev);

    return 0;
}
