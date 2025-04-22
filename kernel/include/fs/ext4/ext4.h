#ifndef __EXT4_H__
#define __EXT4_H__

#include <fs/ext4/lwext4/ext4.h>

struct fs_dev {
    struct ext4_blockdev ext4_blkdev;
    struct ext4_blockdev_iface ext4_blkdev_if;
    struct ext4_blockdev *blkdev;
};

// extern const struct inode_operations ext4_file_iops;
extern const struct file_operations ext4_file_fops;
extern const struct fs_operations ext4_filesystem_ops;
extern const struct file_system ext4_fs;

#endif // __EXT4_H__