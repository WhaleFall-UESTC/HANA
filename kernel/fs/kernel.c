#include <fs/kernel.h>
#include <fs/fcntl.h>
#include <fs/mountp.h>
#include <mm/mm.h>
#include <debug.h>
#include <lib/errno.h>

#define KERNEL_OPEN_FLAG O_RDONLY

struct file* kernel_open(const char* path) {
    struct mountpoint *mount_p = NULL;
    int ret;
    struct file *file = NULL;
	struct inode *inode = NULL;
    char full_path[MAX_PATH_LEN];

    ret = get_absolute_path(path, full_path, AT_FDCWD);
	if (ret < 0)
	{
		error("get absolute path error");
		return ERR_PTR(-EINVAL);
	}

    mount_p = mountpoint_find(full_path);
	if (mount_p == NULL)
	{
		error("mountpoint not found for path %s", full_path);
		goto out_err;
	}

    file = kcalloc(sizeof(struct file), 1);
	if (file == NULL)
	{
		error("alloc file error");
		goto out_err;
	}

	inode = kcalloc(sizeof(struct inode), 1);
	if (inode == NULL)
	{
		error("alloc inode error");
		goto out_file;
	}

	file_init(file, NULL, full_path, KERNEL_OPEN_FLAG, NULL);

    ret = call_interface(mount_p->fs->fs_op, ifget, int, mount_p, inode, file);
	if (ret < 0)
	{
		error("ifget error");
		goto out_inode;
	}

    ret = call_interface(file->f_op, openat, int, file, full_path, KERNEL_OPEN_FLAG, 0);
	if (ret != 0)
	{
		error("open error, ret: %d", ret);
		goto out_inode;
	}

    file_get(file);
    return file;

out_inode:
	kfree(inode);
out_file:
	kfree(file);
out_err:
	return NULL;
}

ssize_t kernel_read(struct file* file, void* buf, size_t size) {
    ssize_t ret;
    off_t ori_fpos;

    ori_fpos = file->fpos;
    ret = call_interface(file->f_op, read, ssize_t, file, buf, size, &file->fpos);
    if (ret < 0) {
        error("read error");
        return -1;
    }

    return file->fpos - ori_fpos;
}

ssize_t kernel_write(struct file* file, const void* buf, size_t size) {
    ssize_t ret;
    off_t ori_fpos;

    ori_fpos = file->fpos;
    ret = call_interface(file->f_op, write, ssize_t, file, buf, size, &file->fpos);
    if (ret < 0) {
        error("write error");
        return -1;
    }

    return file->fpos - ori_fpos;
}

int kernel_close(struct file* file) {
    int ret;

    ret = file_put(file);
    if (ret < 0) {
        error("close error");
        return -1;
    }

    return 0;
}

off_t kernel_lseek(struct file* file, off_t offset, int whence) {
	int ret;

    ret = call_interface(file->f_op, llseek, int, file, offset, whence);
	if (ret < 0)
		return -1;

	file->fpos = ret;

	return ret;
}

int kernel_mount(const char * special, const char * dir, const char * fstype, unsigned long flags, const void * data) {
	int len;
	KCALLOC(struct mountpoint, mp, 1);
	struct blkdev *blkdev;

	len = str_match_prefix(special, "/dev/");

	if (len != 5)
	{
		error("cannot find special device.");
		return -1;
	}

	struct devfs_device* device = devfs_get_by_path(special);
	if (device == NULL || device->file_type != FT_BLKDEV)
	{
		error("block device %s not found", special + len);
		return -1;
	}
	blkdev = device->disk.blkdev;

	if (mountpoint_find(dir) != NULL)
	{
		error("Mount point already used.");
		return -1;
	}

	mp->blkdev = blkdev;
	mp->device = device;
	mp->fs = filesys_find(fstype);
	mp->mountpoint = strdup(dir);

	mountpoint_add(mp);

	return call_interface(mp->fs->fs_op, mount, int, blkdev, mp, data);
}