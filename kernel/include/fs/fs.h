#ifndef __FS_H__
#define __FS_H__

#include <common.h>
#include <io/blk.h>

#include <lwext4/ext4.h>
#include <lwext4/ext4_blockdev.h>

/**
 * File system device structure.
 */
struct fs_dev {
    struct ext4_blockdev ext4_blkdev;
    struct ext4_blockdev_iface ext4_blkdev_if;
    struct blkdev *blkdev;
};

#define get_blkdev_from_blkext4(bdev) (container_of((bdev), struct fs_dev, ext4_blkdev)->blkdev)

#define debug_ext4(fmt, args...) \
    debug("[Ext4] " fmt, ##args)
#define error_ext4(fmt, args...) \
    error("[Ext4] " fmt, ##args)

int fs_blk_mount_ext4(struct blkdev* blkdev, const char* mountpoint);
int dir_open(struct ext4_dir *dir, const char *path);
int fclose(struct ext4_file *file);
int fname(struct ext4_file *f, char *path);
int fopen(struct ext4_file *file, const char *path, uint32_t flags);
int fread(struct ext4_file *file, uint64 buf, uint off, uint size, int *rcnt);
int fseek(struct ext4_file *file, uint off, uint origin);
int fkstatat(char *path, struct kstat *kst, int dirfd);
int funlink(const char *path);
int fwrite(struct ext4_file *file, uint64 buf, uint off, uint size, int *wcnt);
int fwritev(struct ext4_file *f, struct iovec iov[], int iovcnt, uint off);
int freadv(struct ext4_file *f, struct iovec iov[], int iovcnt, uint off);
uint32 get_inode_type(struct ext4_file *file);
int mkdir(const char *path);
int dir_close(struct ext4_dir *dir);

#endif // __FS_H__