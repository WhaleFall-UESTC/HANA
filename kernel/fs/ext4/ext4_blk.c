/*
 * Copyright (c) 2015 Grzegorz Kostka (kostka.grzegorz@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <fs/ext4/lwext4/ext4_config.h>
#include <fs/ext4/lwext4/ext4_blockdev.h>
#include <fs/ext4/lwext4/ext4_errno.h>
#include <fs/ext4/ext4.h>

#include <io/blk.h>
#include <mm/mm.h>
#include <debug.h>
#include <fs/fs.h>

#define get_blkdev_from_blkext4(bdev) \
	container_of(bdev, struct ext4_fs_dev, ext4_blkdev)->blkdev

int blockdev_open(struct ext4_blockdev *bdev)
{
	/**
	 * For now this function needs no implementation
	 */
	// debug("blockdev_open");
	return EOK;
}

int blockdev_bread(struct ext4_blockdev *bdev, void *buf, uint64 blk_id,
			 uint32 blk_cnt)
{
	struct blkdev* blkdev = get_blkdev_from_blkext4(bdev);
	struct blkreq* req;
	int ret = EOK;

	assert(blkdev != NULL);

	debug("blockdev_bread: %s", blkdev->name);

	req = blkreq_alloc(blkdev, blk_id, (void *)buf,
			   blk_cnt * blkdev->sector_size, 0);
	if(req == NULL) {
		error_ext4("Failed to allocate block request");
		ret = EIO;
		goto out;
	}

	blkdev_submit_req_wait(blkdev, req);

	if(req->status != BLKREQ_STATUS_OK) {
		error_ext4("Failed to read from block device");
		ret = EIO;
		goto out_free;
	}

out_free:
	blkreq_free(blkdev, req);

out:
	debug("bread finished, ret = %d", ret);
	return ret;
}

int blockdev_bwrite(struct ext4_blockdev *bdev, const void *buf,
			  uint64 blk_id, uint32 blk_cnt)
{
	struct blkdev* blkdev = get_blkdev_from_blkext4(bdev);
	struct blkreq* req;
	int ret = EOK;

	assert(blkdev != NULL);

	debug("blockdev_bwrite: %s", blkdev->name);

	req = blkreq_alloc(blkdev, blk_id, (void *)buf,
			   blk_cnt * blkdev->sector_size, 1);
	if(req == NULL) {
		error_ext4("Failed to allocate block request");
		ret = EIO;
		goto out;
	}

	blkdev_submit_req_wait(blkdev, req);

	if(req->status != BLKREQ_STATUS_OK) {
		error_ext4("Failed to read from block device");
		ret = EIO;
		goto out_free;
	}

out_free:
	blkreq_free(blkdev, req);

out:
	debug("bwrite finished, ret = %d", ret);
	return ret;
}

int blockdev_close(struct ext4_blockdev *bdev)
{
	/**
	 * For now this function needs no implementation
	 */
	// debug("blockdev_close");
	return EOK;
}

int blockdev_lock(struct ext4_blockdev *bdev)
{
	/**
	 * For now this function needs no implementation
	 */
	// debug("blockdev_lock");
	return EOK;
}

int blockdev_unlock(struct ext4_blockdev *bdev)
{
	/**
	 * For now this function needs no implementation
	 */
	// debug("blockdev_unlock");
	return EOK;
}


