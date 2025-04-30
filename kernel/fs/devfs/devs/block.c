#include <fs/devfs/devs/block.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <mm/mm.h>

ssize_t block_read(struct disk *disk, char *buffer, size_t size, off_t *offset)
{
	struct blkdev *blkdev;
	uint32 blk_cnt, blk_id, i;
	struct blkreq *req;
	char *buf_ptr;
	ssize_t ret = 0;
	int nr;
	bool incomplete;

	assert(disk != NULL);

	blkdev = disk->blkdev;

	if (size == 0)
		return 0;
	blk_cnt = (size - 1) / blkdev->sector_size + 1;
	blk_id = *offset / blkdev->sector_size;
	incomplete = !!(size % blkdev->sector_size);

	buf_ptr = buffer;
	for (i = 0; i < blk_cnt; i++)
	{
		if (incomplete && i == blk_cnt - 1)
		{
			memset(disk->buffer, 0, blkdev->sector_size);
			req = blkreq_alloc(blkdev, blk_id + i, (void *)disk->buffer, blkdev->sector_size, BLKREQ_READ);
		}
		else
		{
			req = blkreq_alloc(blkdev, blk_id + i, (void *)buf_ptr, blkdev->sector_size, BLKREQ_READ);
		}

		if (req == NULL)
		{
			error("Failed to allocate block request");
			ret = -1;
			goto out;
		}

		blkdev_submit_req(blkdev, req);

		if (incomplete && i < blk_cnt - 2)
		{
			buf_ptr += blkdev->sector_size;
		}
	}

out:
	nr = blkdev_wait_all(blkdev);
	if (!ret && nr)
		ret = -1;

	if (incomplete)
	{
		memcpy(buf_ptr, disk->buffer, size % blkdev->sector_size);
	}

	blkdev_free_all(blkdev);
	*offset += size;
	return ret;
}

ssize_t block_write(struct disk *disk, const char *buffer, size_t size, off_t *offset)
{
	struct blkdev *blkdev;
	uint32 blk_cnt, blk_id, i;
	struct blkreq *req;
	const char *buf_ptr;
	ssize_t ret = 0;
	int nr;
	bool incomplete;

	assert(disk != NULL);

	blkdev = disk->blkdev;

	if (size == 0)
		return 0;
	blk_cnt = (size - 1) / blkdev->sector_size + 1;
	blk_id = *offset / blkdev->sector_size;
	incomplete = !!(size % blkdev->sector_size);

	if (incomplete)
	{
		memset(disk->buffer, 0, blkdev->sector_size);
	}

	buf_ptr = buffer;
	for (i = 0; i < blk_cnt; i++)
	{
		if (incomplete && i == blk_cnt - 1)
		{
			memcpy(disk->buffer, buf_ptr, size % blkdev->sector_size);
			req = blkreq_alloc(blkdev, blk_id + i, (void *)disk->buffer, blkdev->sector_size, BLKREQ_WRITE);
		}
		else
		{
			req = blkreq_alloc(blkdev, blk_id + i, (void *)buf_ptr, blkdev->sector_size, BLKREQ_WRITE);
		}

		if (req == NULL)
		{
			error("Failed to allocate block request");
			ret = -1;
			goto out;
		}

		blkdev_submit_req(blkdev, req);

		if (incomplete && i < blk_cnt - 2)
		{
			buf_ptr += blkdev->sector_size;
		}
	}

out:
	nr = blkdev_wait_all(blkdev);
	if (!ret && nr)
		ret = -1;

	blkdev_free_all(blkdev);
	*offset += size;
	return ret;
}

off_t block_llseek(struct disk *disk, off_t offset, int whence)
{
	struct blkdev *blkdev;
	off_t res_off;

	assert(disk != NULL);

	blkdev = disk->blkdev;

	switch (whence)
	{
	case SEEK_CUR:
		res_off = ((disk->ofs + offset) + blkdev->size) % blkdev->size;
		break;

	case SEEK_END:
	case SEEK_SET:
		res_off = offset;
		break;

	default:
		error("Invalid whence");
		return -1;
	}

	disk->ofs = res_off;
	return res_off;
}

void block_init(struct disk* disk, struct blkdev* blkdev, const char* name) {
    assert(disk != NULL);

    disk->blkdev = blkdev;
    strncpy(disk->name, name, MAX_FILENAME_LEN);
    disk->buffer = kalloc(blkdev->sector_size);
    disk->ofs = 0;
}