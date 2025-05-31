#ifndef __VIRTIO_H__
#define __VIRTIO_H__

#include <common.h>
#include <io/blk.h>

#define VIRTIO_DEV_NET 0x1
#define VIRTIO_DEV_BLK 0x2
#define wrap(x, len) ((x) & ~(len))

#define VIRTIO_STATUS_ACKNOWLEDGE (1)
#define VIRTIO_STATUS_DRIVER (2)
#define VIRTIO_STATUS_FAILED (128)
#define VIRTIO_STATUS_FEATURES_OK (8)
#define VIRTIO_STATUS_DRIVER_OK (4)
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET (64)

#define VIRTIO_BLK_F_RO 5          /* Disk is read-only */
#define VIRTIO_BLK_F_SCSI 7        /* Supports scsi command passthru */
#define VIRTIO_BLK_F_CONFIG_WCE 11 /* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ 12         /* support more than one vq */
#define VIRTIO_F_ANY_LAYOUT 27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX 29

struct virtio_cap
{
    char *name;
    uint32 bit;
    bool support;
    char *help;
};

struct virtqueue_desc
{
    uint64 addr;
    uint32 len;
/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT 1
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE 2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4
    /* The flags as indicated above. */
    uint16 flags;
    /* Next field if flags & NEXT */
    uint16 next;
} __attribute__((packed));

struct virtqueue_avail
{
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
    uint16 flags;
    uint16 idx;
    uint16 ring[0]; // Queuesize of nr elements
} __attribute__((packed));

struct virtqueue_used_elem
{
    uint32 id;
    uint32 len;
} __attribute__((packed));

struct virtqueue_used
{
#define VIRTQ_USED_F_NO_NOTIFY 1
    uint16 flags;
    uint16 idx;
    struct virtqueue_used_elem ring[0]; // Queuesize of nr elements
} __attribute__((packed));

/**
 * Lagacy interfaces force to lay out the virtqueue
 * on two or more physically-contiguous pages.
 * See 2.4.2 Legacy Interfaces: A Note on Virtqueue Layout
 */

#define VIRTIO_DEFAULT_QUEUE_SIZE 128
#define VIRTIO_DEFAULT_ALIGN PGSIZE
#define VIRTIO_DEFAULT_QUEUE_STRUCT_SIZE 8192
#define VIRTIO_DEFAULT_QUEUE_PADDING                                \
    (                                                               \
        VIRTIO_DEFAULT_ALIGN -                                      \
        sizeof(struct virtqueue_desc) * VIRTIO_DEFAULT_QUEUE_SIZE - \
        sizeof(struct virtqueue_avail))

#define QALIGN(x) (((x) + (VIRTIO_DEFAULT_ALIGN - 1)) & (~(VIRTIO_DEFAULT_ALIGN - 1)))
static inline unsigned virtq_size(unsigned int qsz)
{
    return QALIGN(sizeof(struct virtqueue_desc) * qsz + sizeof(uint16) * (2 + qsz)) + QALIGN(sizeof(struct virtqueue_used_elem) * qsz);
}

struct virtqueue
{
    // The actual descriptors (16 bytes each)
    struct virtqueue_desc desc[VIRTIO_DEFAULT_QUEUE_SIZE];

    // A ring of available descriptor heads with free-running index.
    struct virtqueue_avail avail;

    // Padding to the next Queue Align boundary.
    uint8 pad[VIRTIO_DEFAULT_QUEUE_PADDING];

    // A ring of used descriptor heads with free-running index.
    struct virtqueue_used used;
} __attribute__((packed));

struct virtq_info
{
    union
    {
        uint64 phyaddr;
        /* Physical page frame number of struct virtqueue. */
        uint64 pfn;
    };

    uint32 seen_used;
    uint32 free_desc;

    volatile struct virtqueue *virtq;
    void *desc_virt[VIRTIO_DEFAULT_QUEUE_SIZE];
};

struct virtio_blk_config
{
    uint64 capacity;
    uint32 size_max;
    uint32 seg_max;
    struct
    {
        uint16 cylinders;
        uint8 heads;
        uint8 sectors;
    } geometry;
    uint32 blk_size;
    struct
    {
        uint8 physical_block_exp;
        uint8 alignment_offset;
        uint16 min_io_size;
        uint32 opt_io_size;
    } topology;
    uint8 writeback;
} __attribute__((packed));

struct virtio_net_config
{
    uint8 mac[6];
#define VIRTIO_NET_S_LINK_UP 1
#define VIRTIO_NET_S_ANNOUNCE 2
    uint16 status;
    uint16 max_virtqueue_pairs;
} __attribute__((packed));

#define VIRTIO_BLK_REQ_HEADER_SIZE 16
#define VIRTIO_BLK_REQ_FOOTER_SIZE 1
struct virtio_blk_req
{
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_SCSI 2
#define VIRTIO_BLK_T_FLUSH 4
    uint32 type;
    uint32 reserved;
    uint64 sector;
    uint8 status;
    /* end standard fields, begin helpers */
    // uint8 _pad[3];
    uint32 descriptor;
    struct blkreq blkreq;
} __attribute__((aligned(4)));
// };

#define VIRTIO_BLK_SECTOR_SIZE 512

#define VIRTIO_BLK_S_OK 0
#define VIRTIO_BLK_S_IOERR 1
#define VIRTIO_BLK_S_UNSUPP 2

struct virtqueue *virtq_create();
uint32 virtq_alloc_desc(struct virtq_info *virtq_info, void *addr);
void virtq_free_desc(struct virtq_info *virtq_info, uint32 desc);
void virtq_show(struct virtq_info *virtq_info);

#define VIRTIO_INDP_CAPS                                              \
    {"VIRTIO_F_RING_INDIRECT_DESC", 28, false,                        \
     "Negotiating this feature indicates that the driver can use"     \
     " descriptors with the VIRTQ_DESC_F_INDIRECT flag set, as"       \
     " described in 2.4.5.3 Indirect Descriptors."},                  \
        {"VIRTIO_F_RING_EVENT_IDX", 29, false,                        \
         "This feature enables the used_event and the avail_event "   \
         "fields"                                                     \
         " as described in 2.4.7 and 2.4.8."},                        \
        {"VIRTIO_F_VERSION_1", 32, false,                             \
         "This indicates compliance with this specification, giving " \
         "a"                                                          \
         " simple way to detect legacy devices or drivers."},

#endif // __VIRTIO_H__