#include <common.h>
#include <debug.h>
#include <mm/mm.h>
#include <drivers/virtio.h>
// #include <locking/spinsem.h>
#include <locking/spinlock.h>
#include <proc/proc.h>
#include <irq/interrupt.h>
#include <io/blk.h>
#include <klib.h>

#ifdef ARCH_RISCV
#include <riscv.h>
#endif

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

struct slab *blkreq_slab = NULL;
struct list_head vdevs;
spinlock_t vdev_list_lock;

struct virtio_blk
{
    virtio_regs *regs;
    struct virtio_blk_config *config;
    struct virtq_info *virtq_info;
    uint32 intid;
    struct list_head list;
    struct blkdev blkdev;
};
#define get_vblkdev(dev) container_of(dev, struct virtio_blk, blkdev)

#define HI32(u64) ((uint32)((0xFFFFFFFF00000000ULL & (u64)) >> 32))
#define LO32(u64) ((uint32)(0x00000000FFFFFFFFULL & (u64)))

static void virtio_blk_handle_used(struct virtio_blk *dev, uint32 usedidx)
{
    struct virtq_info* virtq_info = dev->virtq_info;
    volatile struct virtqueue *virtq = virtq_info->virtq;
    uint32 desc1, desc2, desc3;
    struct virtio_blk_req *req;

    log("virtio_blk_handle_used: usedidx=%u, usedid=%u\n",
           usedidx, virtq->used.ring[usedidx].id);

    desc1 = virtq->used.ring[usedidx].id;
    if (!(virtq->desc[desc1].flags & VIRTQ_DESC_F_NEXT))
        goto bad_desc;
    desc2 = virtq->desc[desc1].next;
    if (!(virtq->desc[desc2].flags & VIRTQ_DESC_F_NEXT))
        goto bad_desc;
    desc3 = virtq->desc[desc2].next;
    if (virtq->desc[desc1].len != VIRTIO_BLK_REQ_HEADER_SIZE ||
        virtq->desc[desc2].len != VIRTIO_BLK_SECTOR_SIZE ||
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
        break;
    case VIRTIO_BLK_S_IOERR:
        req->blkreq.status = BLKREQ_STATUS_ERR;
        break;
    default:
        panic("Unhandled status in virtio_blk irq\n");
    }

    // wait_list_awaken(&req->blkreq.wait);
    wakeup(blkreq_wait_channel(&req->blkreq));
    return;
bad_desc:
    log("virtio-blk received malformed descriptors\n");
    return;
}

static struct virtio_blk *virtio_blk_get_dev_by_intid(uint32 intid)
{
    struct virtio_blk *blk;
    // int flags;
    // spin_acquire_irqsave(&vdev_list_lock, &flags);
    spinlock_acquire(&vdev_list_lock);
    list_for_each_entry(blk, &vdevs, list)
    {
        if (blk->intid == intid)
        {
            spinlock_release(&vdev_list_lock);
            // spin_release_irqrestore(&vdev_list_lock, &flags);
            return blk;
        }
    }
    spinlock_release(&vdev_list_lock);
    // spin_release_irqrestore(&vdev_list_lock, &flags);
    return NULL;
}

static irqret_t virtio_blk_isr(uint32 intid, void* private)
{
    int i;
    struct virtio_blk *dev = virtio_blk_get_dev_by_intid(intid);
    struct virtq_info* virtq_info = dev->virtq_info;

    log("irq triggered, intid=%u\n", intid);

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
    
    WRITE32(dev->regs->InterruptACK, READ32(dev->regs->InterruptStatus) & 0x3);
    
    irq_free(intid);
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
    log("virtio_blk_dev at 0x%lx\n",
           virt_to_phys((uint64)blkdev->regs));
    log("    Status=0x%x\n", READ32(blkdev->regs->Status));
    log("    DeviceID=0x%x\n", READ32(blkdev->regs->DeviceID));
    log("    VendorID=0x%x\n", READ32(blkdev->regs->VendorID));
    log("    InterruptStatus=0x%x\n",
           READ32(blkdev->regs->InterruptStatus));
    log("    MagicValue=0x%x\n", READ32(blkdev->regs->MagicValue));
    log("  Queue 0:\n");
    log("    avail.idx = %u\n", virtq->avail.idx);
    log("    used.idx = %u\n", virtq->used.idx);
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

    log("virtio_blk_submit: %s, sector=%lu, size=%lu\n",
        req->type == BLKREQ_TYPE_READ ? "read" : "write",
        req->sector_sta, req->size);

    d1 = virtq_alloc_desc(virtq_info, hdr);
    hdr->descriptor = d1;
    virtq->desc[d1].len = VIRTIO_BLK_REQ_HEADER_SIZE;
    virtq->desc[d1].flags = VIRTQ_DESC_F_NEXT;

    d2 = virtq_alloc_desc(virtq_info, req->buffer);
    virtq->desc[d2].len = VIRTIO_BLK_SECTOR_SIZE;
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

static void virtio_mod_init(void)
{
    INIT_LIST_HEAD(vdevs);
    spinlock_init(&vdev_list_lock, "virtio-blk spinlock");
}

struct blkdev_ops virtio_blk_ops = {
    .alloc = virtio_blk_alloc,
    .free = virtio_blk_free,
    .submit = virtio_blk_submit,
    .status = virtio_blk_status,
};

int virtio_blk_init(volatile virtio_regs *regs, uint32 intid)
{
    struct virtio_blk *vdev;
    struct virtq_info *virtq_info;
    uint64 blk_size, _blk_size;

    virtio_mod_init();
    vdev = kalloc(sizeof(struct virtio_blk));

    // Read and write feature bits
    // virtio_check_capabilities(regs, blk_caps, nr_elem(blk_caps), "virtio-blk");
    virtio_check_capabilities(regs);

    uint64 features = READ32(regs->HostFeatures);
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    WRITE32(regs->GuestFeatures, features);
    mb();

    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_FEATURES_OK);
	mb();
	if (!(regs->Status & VIRTIO_STATUS_FEATURES_OK)) {
		error("virtio-blk did not accept our features\n");
		return -1;
	}

    WRITE32(regs->GuestPageSize, PGSIZE);
    mb();

    // Perform device-specific setup
    virtq_info = virtq_add_to_device(regs, 0);
    assert(virtq_info != NULL);

    vdev->regs = regs;
    vdev->virtq_info = virtq_info;
    vdev->intid = intid;
    vdev->config = (struct virtio_blk_config *)&regs->Config;
    vdev->blkdev.ops = &virtio_blk_ops;

    // Read device configuration fields
    blk_size = READ64(vdev->config->capacity);

    do {
        _blk_size = blk_size;
        mb();
        blk_size = READ64(vdev->config->capacity);
    } while(blk_size != _blk_size);

    vdev->blkdev.size = blk_size * VIRTIO_BLK_SECTOR_SIZE;

    snprintf(vdev->blkdev.name, sizeof(vdev->blkdev.name), "vblk%d", vdev->intid);

    log("virtio-blk: %s, size=%lu, intid=%d\n", vdev->blkdev.name, vdev->blkdev.size, vdev->intid);

    spinlock_acquire(&vdev_list_lock);
    list_insert(&vdevs, &vdev->list);
    spinlock_release(&vdev_list_lock);

    irq_register(intid, virtio_blk_isr, NULL);

    // Set DRIVER_OK status bit
    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_DRIVER_OK);
    mb();

    blkdev_init(&vdev->blkdev);
    blkdev_register(&vdev->blkdev);

    return 0;
}
