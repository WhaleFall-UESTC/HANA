#ifndef __EXT4_BLK__
#define __EXT4_BLK__

#include "lwext4/ext4_config.h"
#include "lwext4/ext4_blockdev.h"

#include <common.h>
#include <stdbool.h>

int blockdev_open(struct ext4_blockdev *bdev);
int blockdev_bread(struct ext4_blockdev *bdev, void *buf, uint64 blk_id,
			 uint32 blk_cnt);
int blockdev_bwrite(struct ext4_blockdev *bdev, const void *buf,
			  uint64 blk_id, uint32 blk_cnt);
int blockdev_close(struct ext4_blockdev *bdev);
int blockdev_lock(struct ext4_blockdev *bdev);
int blockdev_unlock(struct ext4_blockdev *bdev);

#endif /* __EXT4_BLK__ */
