#include <io/blk.h>
#include <fs/io.h>
#include <fs/fs.h>
#include <fs/fcntl.h>
#include <fs/stat.h>
#include <fs/dcache.h>
#include <fs/file.h>

#include <fs/ext4/ext4.h>
#include <fs/ext4/ext4_blk.h>

#include <fs/ext4/lwext4/ext4.h>
#include <fs/ext4/lwext4/ext4_fs.h>
#include <fs/ext4/lwext4/ext4_errno.h>

#define MAX_EXT4_BLOCKDEV_NAME 32
#define EXT4_BUF_SIZE 512

int
ext4_fs_mount(struct blkdev * blkdev, const char *mountpoint)
{
	char buffer[MAX_EXT4_BLOCKDEV_NAME + 1];
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
		error_ext4("device register error! ret = %d", ret);
		return -1;
	}

	ret = ext4_mount(buffer, mountpoint, false);
	if (ret != EOK)
	{
		error_ext4("mount error! ret = %d", ret);
		return -1;
	}

	ret = ext4_cache_write_back(mountpoint, true);
	if (ret != EOK)
	{
		error_ext4("cache write back error! ret = %d", ret);
		return -1;
	}

	return 0;
}

int ext4_fs_ifget(struct inode* inode, struct file* file) {
	if ((file->f_flags & O_DIRECTORY) == O_DIRECTORY) {
		struct ext4_dir *dir;
		dir = (struct ext4_dir *)kalloc(sizeof(struct ext4_dir));
		if (dir == NULL) {
			error_ext4("ext4_dir alloc error!");
			return -1;
		}
		file->f_private = (void*)dir;
	} else {
		struct ext4_file *ext4_file = (struct ext4_file *)kalloc(sizeof(struct ext4_file));
		if (ext4_file == NULL) {
			error_ext4("ext4_file alloc error!");
			return -1;
		}
		file->f_private = (void*)ext4_file;
	}

	inode->i_op = &ext4_file_iops;
	file->f_op = &ext4_file_fops;
	file->f_inode = inode;

	return 0;
}

// int fname(struct ext4_file *f, char *path)
// {

//   int ret;
//   if((ret = fopen(f, path, O_RDONLY)) != EOK) {
//     // error_ext4("fopen error! ret: %d", ret);
//     return ret;
//   }

//   return ret;
// }

static off_t ext4_llseek(struct file* file, off_t offset, int whence) {
	struct ext4_file *ext4_file = (struct ext4_file *)file->f_private;
	int ret;
	
	ret = ext4_fseek(ext4_file, offset, whence);
	if (ret != EOK) {
		error_ext4("ext4_fseek error! ret: %d", ret);
		return -1;
	}

	return offset;
}

static ssize_t ext4_read(struct file* file, char * buffer, size_t size, off_t * offset) {
	struct ext4_file *ext4_file = (struct ext4_file *)file->f_private;
	int ret;
	size_t rcnt;

	ret = ext4_fseek(ext4_file, offset, SEEK_SET);
	if (ret != EOK) {
		error_ext4("ext4_fseek error! ret: %d", ret);
		return -1;
	}

	ret = ext4_fread(ext4_file, (void*)buffer, size, &rcnt);
	if (ret != EOK) {
		error_ext4("ext4_fread error! ret: %d", ret);
		return -1;
	}

	*offset += rcnt;
	return rcnt;
}

static ssize_t ext4_write(struct file* file, const char * buffer, size_t size, off_t * offset) {
	struct ext4_file *ext4_file = (struct ext4_file *)file->f_private;
	int ret;
	size_t wcnt;

	ret = ext4_fseek(ext4_file, offset, SEEK_SET);
	if (ret != EOK) {
		error_ext4("ext4_fseek error! ret: %d", ret);
		return -1;
	}

	ret = ext4_fwrite(ext4_file, (void*)buffer, size, &wcnt);
	if (ret != EOK) {
		error_ext4("ext4_fwrite error! ret: %d", ret);
		return -1;
	}

	*offset += wcnt;
	return wcnt;
}

static int ext4_open(struct file* file, struct path* path, uint32 flags) {
	struct ext4_file *ext4_file;
	struct ext4_dir *dir;
	int ret;

	if(S_ISDIR(file->f_inode->i_mode)){
		/**
		 * TODO: it seems not beautiful to check it here
		 */
		if((flags & O_DIRECTORY) != O_DIRECTORY) {
			error_ext4("open directory without a directory flag");
			return -1;
		}

		dir = (struct ext4_dir *)file->f_private;

		ret = ext4_dir_open(dir, path);

		if (ret != EOK) {
			error_ext4("ext4_dir_open error! ret: %d", ret);
			return -1;
		}
	}
	else {
		ext4_file = (struct ext4_file *)file->f_private;

		ret = ext4_fopen2(ext4_file, path->p_name, flags);
		if (ret != EOK) {
			error_ext4("ext4_fopen2 error! ret: %d", ret);
			return -1;
		}
	}

	return ret;
}

static int ext4_close(struct file* file) {
	struct ext4_file *ext4_file;
	struct ext4_dir *dir;
	int ret;

	if(S_ISDIR(file->f_inode->i_mode)){
		dir = (struct ext4_dir *)file->f_private;

		ret = ext4_dir_close(dir);
		if (ret != EOK) {
			error_ext4("ext4_dir_close error! ret: %d", ret);
			return -1;
		}
	}
	else {
		ext4_file = (struct ext4_file *)file->f_private;

		ret = ext4_fclose(ext4_file);
		if (ret != EOK) {
			error_ext4("ext4_fclose error! ret: %d", ret);
			return -1;
		}
	}

	return ret;
}

static int ext4_symlink(struct inode* inode, struct path* path, struct path* softlink_path) {
	int ret = ext4_fsymlink(path->p_name, softlink_path->p_name);
	return ret;
}

static int ext4_link(struct path* path, struct inode* inode, struct path *hardlink_path) {
	int ret = ext4_flink(path->p_name, hardlink_path->p_name);
	return ret;
}

static int ext4_unlink(struct inode* inode, struct path* path) {
	int ret = ext4_fremove(path->p_name);
	return ret;
}

// int readlink(const char *path, char *buf, size_t bufsize, size_t *rcnt)
// {
// 	int ret = ext4_readlink(path,buf,bufsize,rcnt);
// 	return ret;
// }

static int ext4_getattr(const struct path * path, struct stat * stat) {
	int ret;
	uint32 ino;
	struct ext4_inode inode;

	assert(stat != NULL);

	if((ret = ext4_raw_inode_fill(path->p_name, &ino, &inode)) != EOK){
		error_ext4("ext4_raw_inode_fill error, ret: %d", ret);
		return -1;
	}

	stat->st_dev = 0;
	stat->st_ino = ino;
	stat->st_mode = inode.mode; /* TODO: fix the error during du, ls */
	stat->st_nlink = inode.links_count;
	stat->st_uid = 0;
	stat->st_gid = 0;
	stat->st_rdev = 0;
	stat->st_size = inode.size_lo;
	stat->st_blksize = 512;
	stat->st_blocks = (uint64)inode.blocks_count_lo;
	stat->st_atime = 0;
	stat->st_atime_nsec = 0;
	stat->st_ctime = 0;
	stat->st_ctime_nsec = 0;
	stat->st_mtime = 0;
	stat->st_mtime_nsec = 0;

	return 0;
}

static int ext4_mkdir(struct inode* inode, struct path* path, umode_t mode) {
	int ret = ext4_dir_mk(path->p_name);
	if(ret != EOK){
		error_ext4("mkdir error! ret=%d", ret);
		return -1;
	}
	return ret;
}

static int ext4_rmdir(struct inode* inode, struct path* path) {
	int ret = ext4_dir_rm(path->p_name);
	if(ret != EOK){
		error_ext4("rmdir error! ret=%d", ret);
		return -1;
	}
	return ret;
}

static int ext4_rename(struct path* path, struct path* new_path) {
	int ret = ext4_frename(path->p_name, new_path->p_name);
	if(ret != EOK){
		error_ext4("rename error! ret=%d", ret);
		return -1;
	}
	return ret;
}

const struct inode_operations ext4_file_iops = {
	.link = ext4_link,
	.unlink = ext4_unlink,
	.symlink = ext4_symlink,
	.mkdir = ext4_mkdir,
	.rmdir = ext4_rmdir,
	.rename = ext4_rename,
	.getattr = ext4_getattr,
};

const struct file_operations ext4_file_fops = {
	.llseek = ext4_llseek,
	.read = ext4_read,
	.write = ext4_write,
	.open = ext4_open,
	.close = ext4_close,
};

const struct file_system ext4_fs = {
	.name = "ext4",
	.mount = ext4_fs_mount,
	.ifget = ext4_fs_ifget,
};