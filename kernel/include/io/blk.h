#ifndef __BLK_H__
#define __BLK_H__

#include <common.h>
#include <list.h>
#include <debug.h>

#define KERNEL_SECTOR_SIZE 512
#define KERNEL_SECTOR_SHIFT 9

struct blkreq {
    // request type
    enum blkreq_type {
        BLKREQ_TYPE_READ,
        BLKREQ_TYPE_WRITE
    } type;

    // request info
    sector_t sector_sta;
    uint64 size;
    /*
        due to consistent page mapping,
        we use single virtual address instead of mutiple pages
    */
    void* buffer;

    // request result
    enum blkreq_status {
        BLKREQ_STATUS_INIT,
        BLKREQ_STATUS_OK,
        BLKREQ_STATUS_ERR
    } status;

    struct list_head request_head;
};

struct blkdev_ops;

#define BLKDEV_NAME_MAX_LEN 16

struct blkdev {
    int devid;
    unsigned long size; // blkdev capacity in bytes
    char name[BLKDEV_NAME_MAX_LEN];
    const struct blkdev_ops *ops;
};

struct blkdev_ops {
    struct blkreq* (*alloc)(struct blkdev*);
    void (*free)(struct blkdev*, struct blkreq*);
    void (*submit)(struct blkdev*, struct blkreq*);
    void (*status)(struct blkdev*);
};

static inline void blkreq_init(struct blkreq *request)
{
    assert(request != NULL);

    INIT_LIST_HEAD(request->request_head);

    request->type = BLKREQ_TYPE_READ;
    request->sector_sta = 0;
    request->size = 0;
    request->buffer = NULL;
    request->status = BLKREQ_STATUS_INIT;
}

struct blkdev* blkdev_alloc(int devid, unsigned long size,
                            const char* name, const struct blkdev_ops *ops);

#endif // __BLK_H__