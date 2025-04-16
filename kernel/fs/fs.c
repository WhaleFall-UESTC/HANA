#include <io/blk.h>
#include <fs/fs.h>
#include <fs/ext4_blk.h>
#include <lwext4/ext4.h>
#include <lwext4/ext4_errno.h>

#define MAX_EXT4_BLOCKDEV_NAME 32
#define EXT4_BUF_SIZE 512

int
fs_blk_mount_ext4(struct blkdev * blkdev, const char *mountpoint)
{
	char *buffer[MAX_EXT4_BLOCKDEV_NAME + 1];
	int ret;
	struct ext4_blockdev *blockdev;
	struct ext4_blockdev_iface* ext4_blockdev_if;

	KALLOC(struct fs_dev, fs_dev);

	if (fs_dev == NULL)
	{
		error_ext4("filesystem device alloc error!");
		return -1;
	}

	blockdev = &fs_dev->ext4_blkdev;
	ext4_blockdev_if = &fs_dev->ext4_blkdev_if;

	ext4_blockdev_if->open = blockdev_open;
	ext4_blockdev_if->bread = blockdev_bread;
	ext4_blockdev_if->bwrite = blockdev_bwrite;
	ext4_blockdev_if->close = blockdev_close;
	ext4_blockdev_if->lock = blockdev_lock;
	ext4_blockdev_if->unlock = blockdev_unlock;
	ext4_blockdev_if->ph_bsize = blkdev->sector_size;
	ext4_blockdev_if->ph_bcnt = blkdev->size / blkdev->sector_size;
	ext4_blockdev_if->ph_bbuf = (uint8_t*)kalloc(EXT4_BUF_SIZE);

	blockdev->bdif = ext4_blockdev_if;
	blockdev->part_offset = 0;
	blockdev->part_size = blkdev->size;

	fs_dev->blkdev = blkdev;

	sprintf(buffer, "ext4_%s", blkdev->name);

	ret = ext4_device_register(blockdev, buffer);
	if (ret != EOK)
	{
		error_ext4("[ext4] device register error! ret = %d", ret);
		return -1;
	}

	ret = ext4_mount(buffer, mountpoint, false);
	if (ret != EOK)
	{
		error_ext4("[ext4] mount error! ret = %d", ret);
		return -1;
	}

	ret = ext4_cache_write_back(mountpoint, true);
	if (ret != EOK)
	{
		error_ext4("[ext4] cache write back error! ret = %d", ret);
		return -1;
	}

	return 0;
}