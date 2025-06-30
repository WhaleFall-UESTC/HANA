#ifndef __EXT4_BLK__
#define __EXT4_BLK__

#include "lwext4/ext4_config.h"
#include "lwext4/ext4_blockdev.h"

#include <common.h>
#include <stdbool.h>

/**
 * Open a block device
 * @param bdev: Block device structure
 * @return Always returns EOK
 */
int blockdev_open(struct ext4_blockdev *bdev);

/**
 * Read blocks from a block device
 * @param bdev: Block device structure
 * @param buf: Buffer to store read data
 * @param blk_id: Starting block index
 * @param blk_cnt: Number of blocks to read
 * @return EOK on success, error code on failure
 */
int blockdev_bread(struct ext4_blockdev *bdev, void *buf, uint64 blk_id, uint32 blk_cnt);

/**
 * Write blocks to a block device
 * @param bdev: Block device structure
 * @param buf: Buffer containing data to write
 * @param blk_id: Starting block index
 * @param blk_cnt: Number of blocks to write
 * @return EOK on success, error code on failure
 */
int blockdev_bwrite(struct ext4_blockdev *bdev, const void *buf, uint64 blk_id, uint32 blk_cnt);

/**
 * Close a block device
 * @param bdev: Block device structure
 * @return Always returns EOK
 */
int blockdev_close(struct ext4_blockdev *bdev);

/**
 * Lock a block device for exclusive access
 * @param bdev: Block device structure
 * @return Always returns EOK
 */
int blockdev_lock(struct ext4_blockdev *bdev);

/**
 * Unlock a block device after exclusive access
 * @param bdev: Block device structure
 * @return Always returns EOK
 */
int blockdev_unlock(struct ext4_blockdev *bdev);


#endif /* __EXT4_BLK__ */
