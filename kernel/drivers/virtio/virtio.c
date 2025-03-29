/**
 * Implements virtio device drivers, particularly mmio ones.
 *
 * Reference:
 *
 * http://docs.oasis-open.org/virtio/virtio/v1.0/cs04/virtio-v1.0-cs04.html
 */

#include <platform.h>
#include <common.h>
#include <drivers/virtio.h>
#include <defs.h>
#include <interrupt.h>
#include <debug.h>
#include <mm.h>

#ifdef ARCH_RISCV
#include <riscv.h>
#else
/* other archs */
#endif

#define VIRTIO_MAGIC 0x74726976
#define VIRTIO_VERSION 0x2
#define VIRTIO_DEV_BLK 0x2
#define wrap(x, len) ((x) & ~(len))

uint8 buffer[512];

struct virtio_cap indp_caps[] = {
    {"VIRTIO_F_RING_INDIRECT_DESC", 1<<28, false,
        "Negotiating this feature indicates that the driver can use"
        " descriptors with the VIRTQ_DESC_F_INDIRECT flag set, as"
        " described in 2.4.5.3 Indirect Descriptors."},
    {"VIRTIO_F_RING_EVENT_IDX", 1<<29, false,
        "This feature enables the used_event and the avail_event fields"
        " as described in 2.4.7 and 2.4.8."},
    /*{"VIRTIO_F_VERSION_1", 1<<32, false,
        "This indicates compliance with this specification, giving a"
        " simple way to detect legacy devices or drivers."},*/
};

struct virtio_cap blk_caps[] = {
    {"VIRTIO_BLK_F_SIZE_MAX", 1<<1, false,
        "Maximum size of any single segment is in size_max."},
    {"VIRTIO_BLK_F_SEG_MAX", 1<<2, false,
        "Maximum number of segments in a request is in seg_max."},
    {"VIRTIO_BLK_F_GEOMETRY", 1<<4, false,
        "Disk-style geometry specified in geometry."},
    {"VIRTIO_BLK_F_RO", 1<<5, false,
        "Device is read-only."},
    {"VIRTIO_BLK_F_BLK_SIZE", 1<<6, false,
        "Block size of disk is in blk_size."},
    {"VIRTIO_BLK_F_FLUSH", 1<<9, false,
        "Cache flush command support."},
    {"VIRTIO_BLK_F_TOPOLOGY", 1<<10, false,
        "Device exports information on optimal I/O alignment."},
    {"VIRTIO_BLK_F_CONFIG_WCE", 1<<11, false,
        "Device can toggle its cache between writeback and "
        "writethrough modes."},
};

struct virtio_blk {
    virtio_regs *regs;
    struct virtqueue *virtq;
    uint32 intid;
} blkdev;

struct virtqueue *virtq_create(uint64 len)
{
    int i;
    uint64 page_virt;
    struct virtqueue *virtq;

    /* compute offsets */
    uint64 off_desc = ALIGN(sizeof(struct virtqueue), 16);
    uint64 off_avail = ALIGN(off_desc + len * sizeof(struct virtqueue_desc), 2);
    uint64 off_used_event = (
            off_avail + sizeof(struct virtqueue_avail)
            + len * sizeof(uint16));
    uint64 off_used = ALIGN(off_used_event + sizeof(uint16), 4);
    uint64 off_avail_event = (
            off_used + sizeof(struct virtqueue_used)
            + len * sizeof(struct virtqueue_used_elem));
    uint64 off_desc_virt = ALIGN(off_avail_event + sizeof(uint16), sizeof(void*));
    uint64 memsize = off_desc_virt + len * sizeof(void*);

    if (memsize > PGSIZE) {
        error("virtq_create: error, too big for a page\n");
        return NULL;
    }
    page_virt = (uint64)kalloc(PGSIZE);

    virtq = (struct virtqueue *)page_virt;
    virtq->phys = virt_to_phys(page_virt);
    virtq->len = len;

    virtq->desc = (struct virtqueue_desc *)(page_virt + off_desc);
    virtq->avail = (struct virtqueue_avail *) (page_virt + off_avail);
    virtq->used_event = (uint16 *) (page_virt + off_used_event);
    virtq->used = (struct virtqueue_used *) (page_virt + off_used);
    virtq->avail_event = (uint16 *) (page_virt + off_avail_event);
    virtq->desc_virt = (void **) (page_virt + off_desc_virt);

    virtq->avail->idx = 0;
    virtq->used->idx = 0;
    virtq->seen_used = virtq->used->idx;
    virtq->free_desc = 0;

    for (i = 0; i < len; i++) {
        virtq->desc[i].next = i + 1;
    }

    return virtq;
}

uint32 virtq_alloc_desc(struct virtqueue *virtq, void *addr)
{
    uint32 desc = virtq->free_desc;
    uint32 next = virtq->desc[desc].next;
    if (next == virtq->len)
        error("ERROR: ran out of virtqueue descriptors\n");
    virtq->free_desc = next;

    virtq->desc[desc].addr = virt_to_phys((uint64)addr);
    virtq->desc_virt[desc] = addr;
    return desc;
}

void virtq_free_desc(struct virtqueue *virtq, uint32 desc)
{
    virtq->desc[desc].next = virtq->free_desc;
    virtq->free_desc = desc;
    virtq->desc_virt[desc] = NULL;
}

void virtq_add_to_device(volatile virtio_regs *regs, struct virtqueue *virtq, uint32 queue_sel)
{
    WRITE32(regs->QueueSel, queue_sel);
    mb();
    WRITE32(regs->QueueNum, virtq->len);
    WRITE32(regs->QueueDescLow, virtq->phys + ((void*)virtq->desc - (void*)virtq));
    WRITE32(regs->QueueDescHigh, 0);
    WRITE32(regs->QueueAvailLow, virtq->phys + ((void*)virtq->avail - (void*)virtq));
    WRITE32(regs->QueueAvailHigh, 0);
    WRITE32(regs->QueueUsedLow, virtq->phys + ((void*)virtq->used - (void*)virtq));
    WRITE32(regs->QueueUsedHigh, 0);
    mb();
    WRITE32(regs->QueueReady, 1);
}

static void virtio_check_capabilities(uint32 *device, uint32 *request, struct virtio_cap *caps, uint32 n)
{
    uint32 i;
    for (i = 0; i < n; i++) {
        if (*device & caps[i].bit) {
            if (caps[i].support) {
                *request |= caps[i].bit;
            } else {
                log("virtio supports unsupported option %s (%s)\n",
                        caps[i].name, caps[i].help);
            }
        }
        *device &= ~caps[i].bit;
    }
}

#define HI32(u64) ((uint32)((0xFFFFFFFF00000000ULL & (u64)) >> 32))
#define LO32(u64) ((uint32)(0x00000000FFFFFFFFULL & (u64)))

static void virtio_blk_handle_used(struct virtio_blk *dev, uint32 usedidx)
{
    struct virtqueue *virtq = dev->virtq;
    uint32 desc1, desc2, desc3;
    struct virtio_blk_req *req;
    uint8 *data;

    desc1 = virtq->used->ring[usedidx].id;
    if (!(virtq->desc[desc1].flags & VIRTQ_DESC_F_NEXT))
        goto bad_desc;
    desc2 = virtq->desc[desc1].next;
    if (!(virtq->desc[desc2].flags & VIRTQ_DESC_F_NEXT))
        goto bad_desc;
    desc3 = virtq->desc[desc2].next;
    if (virtq->desc[desc1].len != VIRTIO_BLK_REQ_HEADER_SIZE
            || virtq->desc[desc2].len != VIRTIO_BLK_SECTOR_SIZE
            || virtq->desc[desc3].len != VIRTIO_BLK_REQ_FOOTER_SIZE)
        goto bad_desc;

    req = virtq->desc_virt[desc1];
    data = virtq->desc_virt[desc2];
    if (req->status != VIRTIO_BLK_S_OK)
        goto bad_status;

    if (req->type == VIRTIO_BLK_T_IN) {
        log("virtio-blk: result: \"%s\"\n", data);
    }

    virtq_free_desc(virtq, desc1);
    virtq_free_desc(virtq, desc2);
    virtq_free_desc(virtq, desc3);
    kfree(req);

    return;
bad_desc:
    error("virtio-blk received malformed descriptors\n");
    return;

bad_status:
    error("virtio-blk: error in command response\n");
    return;
}

static irqret_t virtio_blk_isr(uint32 intid, void* private)
{
    /* TODO: support multiple block devices by examining intid */
    struct virtio_blk *dev = (struct virtio_blk *)private;
    int i;
    int len = dev->virtq->len;

    WRITE32(dev->regs->InterruptACK, READ32(dev->regs->InterruptStatus));

    for (i = dev->virtq->seen_used; i != dev->virtq->used->idx; i = wrap(i + 1, len)) {
        virtio_blk_handle_used(dev, i);
    }
    dev->virtq->seen_used = dev->virtq->used->idx;

    return IRQ_HANDLED;
}

static int virtio_blk_init(virtio_regs *regs, uint32 intid)
{
    volatile struct virtio_blk_config *conf = (struct virtio_blk_config*)regs->Config;
    struct virtqueue *virtq;
    uint32 request_features = 0;
    uint32 DeviceFeatures;

    WRITE32(regs->DeviceFeaturesSel, 0);
    WRITE32(regs->DriverFeaturesSel, 0);
    mb();
    DeviceFeatures = regs->DeviceFeatures;
    virtio_check_capabilities(&DeviceFeatures, &request_features, blk_caps, nr_elem(blk_caps));
    virtio_check_capabilities(&DeviceFeatures, &request_features, indp_caps, nr_elem(indp_caps));

    if (DeviceFeatures) {
        log("virtio supports undocumented options 0x%x!\n", DeviceFeatures);
    }

    WRITE32(regs->DriverFeatures, request_features);
    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_FEATURES_OK);
    mb();
    if (!(regs->Status & VIRTIO_STATUS_FEATURES_OK)) {
        puts("error: virtio-blk did not accept our features\n");
        return -1;
    }

    log("virtio-blk has 0x%x %x sectors\n", HI32(conf->capacity), LO32(conf->capacity));
    log("virtio-blk queuenummax %u\n", READ32(regs->QueueNumMax));
    log("virtio-blk Status %x\n", READ32(regs->Status));
    log("virtio-blk InterruptStatus %x\n", regs->InterruptStatus);

    virtq = virtq_create(128);
    virtq_add_to_device(regs, virtq, 0);

    blkdev.regs = regs;
    blkdev.virtq = virtq;
    blkdev.intid = intid;

    register_irq(intid, virtio_blk_isr, &blkdev);

    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_DRIVER_OK);
    mb();
    log("virtio-blk Status %x\n", READ32(regs->Status));

    log("virtio-blk 0x%lx (intid %u): ready!\n", virt_to_phys((uint64)regs), intid);

    return VIRTIO_BLK_S_OK;
}

static int virtio_blk_cmd(struct virtio_blk *blk, uint32 type, uint32 sector, uint8 *data)
{
    struct virtio_blk_req *hdr = kalloc(sizeof(struct virtio_blk_req));
    uint32 d1, d2, d3, datamode = 0;

    hdr->type = type;
    hdr->sector = sector;

    d1 = virtq_alloc_desc(blk->virtq, hdr);
    blk->virtq->desc[d1].len = VIRTIO_BLK_REQ_HEADER_SIZE;
    blk->virtq->desc[d1].flags = VIRTQ_DESC_F_NEXT;

    if (type == VIRTIO_BLK_T_IN) /* if it's a read */
        datamode = VIRTQ_DESC_F_WRITE; /* mark page writeable */

    d2 = virtq_alloc_desc(blk->virtq, data);
    blk->virtq->desc[d2].len = VIRTIO_BLK_SECTOR_SIZE;
    blk->virtq->desc[d2].flags = datamode | VIRTQ_DESC_F_NEXT;

    d3 = virtq_alloc_desc(blk->virtq, (void*)hdr + VIRTIO_BLK_REQ_HEADER_SIZE);
    blk->virtq->desc[d3].len = VIRTIO_BLK_REQ_FOOTER_SIZE;
    blk->virtq->desc[d3].flags = VIRTQ_DESC_F_WRITE;

    blk->virtq->desc[d1].next = d2;
    blk->virtq->desc[d2].next = d3;

    blk->virtq->avail->ring[blk->virtq->avail->idx] = d1;
    mb();
    blk->virtq->avail->idx += 1;
    mb();
    WRITE32(blk->regs->QueueNotify, 0);

    return VIRTIO_BLK_S_OK;
}

static int virtio_blk_read(struct virtio_blk *blk, uint32 sector, uint8 *data)
{
    return virtio_blk_cmd(blk, VIRTIO_BLK_T_IN, sector, data);
}

static int virtio_blk_write(struct virtio_blk *blk, uint32 sector, uint8 *data)
{
    return virtio_blk_cmd(blk, VIRTIO_BLK_T_OUT, sector, data);
}

static int virtio_dev_init(uint64 virt, uint32 intid)
{
    virtio_regs *regs = (virtio_regs *) virt;

    if (READ32(regs->MagicValue) != VIRTIO_MAGIC) {
        error("error: virtio at 0x%lx had wrong magic value 0x%x, expected 0x%x\n",
                virt, regs->MagicValue, VIRTIO_MAGIC);
        return -1;
    }
    if (READ32(regs->Version) != VIRTIO_VERSION) {
        error("error: virtio at 0x%lx had wrong version 0x%x, expected 0x%x\n",
                virt, regs->Version, VIRTIO_VERSION);
        return -1;
    }
    if (READ32(regs->DeviceID) == 0) {
        /*On QEMU, this is pretty common, don't print a message */
        log("warn: virtio at 0x%lx has DeviceID=0, skipping\n", virt);
        return -1;
    }

    /* First step of initialization: reset */
    WRITE32(regs->Status, 0);
    mb();
    /* Hello there, I see you */
    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_ACKNOWLEDGE);
    mb();

    /* Hello, I am a driver for you */
    WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_DRIVER);
    mb();

    switch (READ32(regs->DeviceID)) {
    case VIRTIO_DEV_BLK:
        return virtio_blk_init(regs, intid);
    default:
        error("unsupported virtio device ID 0x%x\n", READ32(regs->DeviceID));
    }

    return VIRTIO_BLK_S_OK;
}

void virtio_blk_cmd_status()
{
    log("virtio_blk_dev at vir 0x%lx\n", (uint64)blkdev.regs);
    log("virtio_blk_dev at 0x%lx\n", virt_to_phys((uint64)blkdev.regs));
    // log("    Status=0x%x\n", READ32(blkdev.regs->Status));
    // log("    DeviceID=0x%x\n", READ32(blkdev.regs->DeviceID));
    // log("    VendorID=0x%x\n", READ32(blkdev.regs->VendorID));
    // log("    InterruptStatus=0x%x\n", READ32(blkdev.regs->InterruptStatus));
    // log("    MagicValue=0x%x\n", READ32(blkdev.regs->MagicValue));
    // log("  Queue 0:\n");
    // log("    avail.idx = %u\n", blkdev.virtq->avail->idx);
    // log("    used.idx = %u\n", blkdev.virtq->used->idx);
    // WRITE32(blkdev.regs->QueueSel, 0);
    // mb();
    // log("    ready = 0x%x\n", READ32(blkdev.regs->QueueReady));
}

// int virtio_blk_cmd_read(int argc, char **argv)
// {
//     uint32 sector;

//     if (argc != 2) {
//         puts("usage: read SECTOR\n");
//         return 1;
//     }

//     sector = atoi(argv[1]);
//     virtio_blk_read(&blkdev, sector, buffer);
//     return 0;
// }

// int virtio_blk_cmd_write(int argc, char **argv)
// {
//     uint32 sector, len;

//     if (argc != 3) {
//         puts("usage: blkwrite SECTOR STRING\n");
//         return 1;
//     }

//     sector = atoi(argv[1]);
//     len = strlen(argv[2]);
//     memcpy(buffer, argv[2], len+1);
//     virtio_blk_write(&blkdev, sector, buffer);
//     return 0;
// }

void virtio_init(void)
{
    uint64 page_virt = VIRT_VIRTIO;

    for (int i = 0; i < 1; i++)
        virtio_dev_init(page_virt + 0x1000 * i, i + 1);

    virtio_blk_cmd_status();
}