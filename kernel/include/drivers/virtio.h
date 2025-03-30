/**
 * virtio declarations (mmio, queue)
 */

#ifndef __VIRTIO_H__
#define __VIRTIO_H__

#include <common.h>
#include <stdbool.h>

/*
* See Section 4.2.2 of VIRTIO 1.0 Spec:
* http://docs.oasis-open.org/virtio/virtio/v1.0/cs04/virtio-v1.0-cs04.html
*/
typedef volatile struct __attribute__((packed)) {
    uint32 MagicValue;
    uint32 Version;
    uint32 DeviceID;
    uint32 VendorID;
    uint32 DeviceFeatures;
    uint32 DeviceFeaturesSel;
    uint32 _reserved0[2];
    uint32 DriverFeatures;
    uint32 DriverFeaturesSel;
    uint32 _reserved1[2];
    uint32 QueueSel;
    uint32 QueueNumMax;
    uint32 QueueNum;
    uint32 _reserved2[2];
    uint32 QueueReady;
    uint32 _reserved3[2];
    uint32 QueueNotify;
    uint32 _reserved4[3];
    uint32 InterruptStatus;
    uint32 InterruptACK;
    uint32 _reserved5[2];
    uint32 Status;
    uint32 _reserved6[3];
    uint32 QueueDescLow;
    uint32 QueueDescHigh;
    uint32 _reserved7[2];
    uint32 QueueAvailLow;
    uint32 QueueAvailHigh;
    uint32 _reserved8[2];
    uint32 QueueUsedLow;
    uint32 QueueUsedHigh;
    uint32 _reserved9[21];
    uint32 ConfigGeneration;
    uint32 Config[0];
} virtio_regs;

#define VIRTIO_STATUS_ACKNOWLEDGE (1)
#define VIRTIO_STATUS_DRIVER (2)
#define VIRTIO_STATUS_FAILED (128)
#define VIRTIO_STATUS_FEATURES_OK (8)
#define VIRTIO_STATUS_DRIVER_OK (4)
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET (64)

struct virtio_cap {
    char *name;
    uint32 bit;
    bool support;
    char *help;
};

struct virtqueue_desc {
    uint64 addr;
    uint32 len;
/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT   1
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE     2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT   4
    /* The flags as indicated above. */
    uint16 flags;
    /* Next field if flags & NEXT */
    uint16 next;
} __attribute__((packed));

struct virtqueue_avail {
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
    uint16 flags;
    uint16 idx;
    uint16 ring[0];
} __attribute__((packed));

struct virtqueue_used_elem {
    uint32 id;
    uint32 len;
} __attribute__((packed));

struct virtqueue_used {
#define VIRTQ_USED_F_NO_NOTIFY 1
    uint16 flags;
    uint16 idx;
    struct virtqueue_used_elem ring[0];
} __attribute__((packed));

/*
* For simplicity, we lay out the virtqueue in contiguous memory on a single
* page. See virtq_create for the layout and alignment requirements.
*/
struct virtqueue {
    /* Physical base address of the full data structure. */
    uint64 phys;
    uint64 len;
    uint32 seen_used;
    uint32 free_desc;

    volatile struct virtqueue_desc *desc;
    volatile struct virtqueue_avail *avail;
    volatile uint16 *used_event;
    volatile struct virtqueue_used *used;
    volatile uint16 *avail_event;
    void **desc_virt;
} __attribute__((packed));

struct virtio_blk_config {
    uint64 capacity;
    uint32 size_max;
    uint32 seg_max;
    struct {
        uint16 cylinders;
        uint8 heads;
        uint8 sectors;
    } geometry;
    uint32 blk_size;
    struct {
        uint8 physical_block_exp;
        uint8 alignment_offset;
        uint16 min_io_size;
        uint32 opt_io_size;
    } topology;
    uint8 writeback;
} __attribute__((packed));

#define VIRTIO_BLK_REQ_HEADER_SIZE 16
#define VIRTIO_BLK_REQ_FOOTER_SIZE 1
struct virtio_blk_req {
#define VIRTIO_BLK_T_IN       0
#define VIRTIO_BLK_T_OUT      1
#define VIRTIO_BLK_T_SCSI     2
#define VIRTIO_BLK_T_FLUSH    4
    uint32 type;
    uint32 reserved;
    uint64 sector;
    uint8 status;
} __attribute__((packed));

#define VIRTIO_BLK_SECTOR_SIZE 512

#define VIRTIO_BLK_S_OK       0
#define VIRTIO_BLK_S_IOERR    1
#define VIRTIO_BLK_S_UNSUPP   2

void virtio_init(void);

#endif // __VIRTIO_H__