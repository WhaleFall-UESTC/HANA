#ifndef __BLK_H__
#define __BLK_H__

#include <common.h>
#include <tools/list.h>
#include <debug.h>
#include <locking/spinlock.h>
#include <irq/interrupt.h>
#include <io/device.h>

#define KERNEL_SECTOR_SIZE 512
#define KERNEL_SECTOR_SHIFT 9

struct blkdev;

/**
 * blkreq is a request for block device
 * one alloc a struct blkreq MUST free it
 * after the request is done
 */
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
    // size MUST be multiple of device SECTOR size
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

#define blkreq_wakeup(req) wakeup(blkreq_wait_channel(req))
#define blkreq_sleep(req) sleep(blkreq_wait_channel(req))

struct blkdev_ops;

struct blkdev
{
    struct device dev; // base device struct
    unsigned long size; // blkdev capacity in bytes
    uint64 sector_size;
    const struct blkdev_ops *ops;
    struct list_head blk_entry; // list entry for block devices
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

static inline struct blkreq* blkreq_alloc(struct blkdev* blkdev, sector_t sector_sta, void* buffer, uint64 size, int write)
{
#define BLKREQ_READ 0
#define BLKREQ_WRITE 1
    struct blkreq* req = blkdev->ops->alloc(blkdev);

    if(req == NULL)
    {
        debug("Failed to allocate block request");
        return NULL;
    }

    if(write)
        req->type = BLKREQ_TYPE_WRITE;
    else
        req->type = BLKREQ_TYPE_READ;

    req->sector_sta = sector_sta;
    req->size = size;
    req->buffer = buffer;

    return req;
}

static inline void blkreq_free(struct blkdev* blkdev, struct blkreq* req)
{
    assert(req != NULL);
    blkdev->ops->free(blkdev, req);
}

/**
 * init block device management system
 */
void block_subsys_init(void);

/**
 * alloc a block device and do initialization
 */
struct blkdev *blkdev_alloc(devid_t devid, unsigned long size, uint64 sector_size,
                            int intr, const char *name, const struct blkdev_ops *ops);

/**
 * initialize a block device
 */
void blkdev_init(struct blkdev *dev, devid_t devid, unsigned long size, uint64 sector_size,
                 int intr, const char *name, const struct blkdev_ops *ops);

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
 * get a blkdev struct by its device id
 */
struct blkdev *blkdev_get_by_id(devid_t id);

/**
 * get default or first blkdev struct of block device
 */
struct blkdev *blkdev_get_default_dev();

/**
 * submit a request to block device
 */
void blkdev_submit_req(struct blkdev *dev, struct blkreq *request);

/**
 * submit a request to block device and wait until it finish
 */
void blkdev_submit_req_wait(struct blkdev *dev, struct blkreq *request);

/**
 * end lift cycle for a blkreq, MUST be called by driver
 * when the request is done
 */
void blkdev_general_endio(struct blkreq *request);

/**
 * wait until all requests in request list done
 * @return: nr of unsuccessful requests
 */
int blkdev_wait_all(struct blkdev *dev);

/**
 * remove and free all requests in given blkdev that are done
 */
void blkdev_free_all(struct blkdev *dev);

/**
 * general setup for block device irq response
 */
irqret_t blkdev_general_isr(uint32 intid, void *private);

#endif // __BLK_H__