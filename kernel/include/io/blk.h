#ifndef __BLK_H__
#define __BLK_H__

#include <common.h>
#include <list.h>
#include <debug.h>
#include <locking/spinlock.h>
#include <irq/interrupt.h>

#define KERNEL_SECTOR_SIZE 512
#define KERNEL_SECTOR_SHIFT 9

struct blkdev;

struct blkreq
{
    // request type
    enum blkreq_type
    {
        BLKREQ_TYPE_READ,
        BLKREQ_TYPE_WRITE
    } type;

    // request info
    sector_t sector_sta;
    uint64 size;
    /**
     * Due to consistent page mapping,
     * we use single virtual address instead of mutiple pages
     * TODO: modify it to page or sector list
     */
    void *buffer;

    // request result
    enum blkreq_status
    {
        BLKREQ_STATUS_INIT,
        BLKREQ_STATUS_OK,
        BLKREQ_STATUS_ERR
    } status;

    struct list_head rq_head;
    struct blkdev *rq_dev;

    /**
     * Callback function per request, registerd by request submitter
     * This function will be called when the request is done,
     * but call time is controlled by guest driver
     * CAN BE NULL
     */
    void (*endio)(struct blkreq *);
};

#define blkreq_wait_channel(req)      \
    ({                                \
        struct blkreq *__req = (req); \
        assert(__req != NULL);        \
        (void *)__req;                \
    })

struct blkdev_ops;

#define BLKDEV_NAME_MAX_LEN 16

struct blkdev
{
    int devid;
    uint32 intr;
    unsigned long size; // blkdev capacity in bytes
    char name[BLKDEV_NAME_MAX_LEN];
    const struct blkdev_ops *ops;
    struct list_head blk_list; // list entry for block devices
    struct list_head rq_list;  // list head for requests
    spinlock_t rq_list_lock;
};

struct blkdev_ops
{
    struct blkreq *(*alloc)(struct blkdev *);
    void (*free)(struct blkdev *, struct blkreq *);
    void (*submit)(struct blkdev *, struct blkreq *);
    void (*status)(struct blkdev *);
    irqret_t (*irq_handle)(struct blkdev *);
};

static inline void blkreq_init(struct blkreq *request, struct blkdev *dev)
{
    assert(request != NULL);

    INIT_LIST_HEAD(request->rq_head);

    request->type = BLKREQ_TYPE_READ;
    request->sector_sta = 0;
    request->size = 0;
    request->buffer = NULL;
    request->status = BLKREQ_STATUS_INIT;
    request->rq_dev = dev;
    request->endio = NULL;
}

/**
 * init block device management system
 */
void blocks_init(void);

/**
 * alloc a block device and do initialization
 */
struct blkdev *blkdev_alloc(int devid, unsigned long size, int intr,
                            const char *name, const struct blkdev_ops *ops);

/**
 * initialize a block device
 */
void blkdev_init(struct blkdev *dev, int devid, int intr, unsigned long size,
                   const char *name, const struct blkdev_ops *ops);

/**
 * register block device in list
 * blkdevs differ by devid(majo&minor)/name
 */
void blkdev_register(struct blkdev *blkdev);

/**
 * get a blkdev struct by its device name
 */
struct blkdev *blkdev_get_by_name(const char *name);

/**
 * submit a request to block device
 */

void blkdev_submit_req(struct blkdev *dev, struct blkreq *request);

/**
 * submit a request to block device and wait until it finish
 */

void blkdev_submit_req_wait(struct blkdev *dev, struct blkreq *request);

/**
 * wait until all requests in request list done
 */
void blkdev_wait_all(struct blkdev *dev);

/**
 * remove and free all requests in given blkdev
 */
void blkdev_free_all(struct blkdev *dev);

/**
 * general setup for block device irq response
 */
irqret_t blkdev_general_isr(uint32 intid, void *private);

#endif // __BLK_H__