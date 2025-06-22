#include <fs/kernel.h>
#include <fs/fcntl.h>
#include <fs/mountp.h>
#include <mm/mm.h>
#include <debug.h>
#include <errno.h>

#define KERNEL_OPEN_FLAG O_RDWR

struct file* kernel_open(const char* path) {
    struct mountpoint *mount_p = NULL;
    int ret;
    struct file *file = NULL;
	struct inode *inode = NULL;

    mount_p = mountpoint_find(path, 0);
	if (mount_p == NULL)
	{
		error("mountpoint not found for path %s", path);
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

	file_init(file, NULL, path, KERNEL_OPEN_FLAG);

    ret = call_interface(mount_p->fs->fs_op, ifget, int, mount_p, inode, file);
	if (ret < 0)
	{
		error("ifget error");
		goto out_inode;
	}

    ret = call_interface(file->f_op, openat, int, file, path, KERNEL_OPEN_FLAG, 0);
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