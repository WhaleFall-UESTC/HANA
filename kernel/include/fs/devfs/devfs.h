#ifndef __DEVFS_H__
#define __DEVFS_H__

#include <fs/devfs/devs/tty.h>
#include <fs/devfs/devs/block.h>
#include <tools/list.h>

struct devfs_device {
    unsigned int file_type;
    
    union {
        struct disk disk;
        struct tty tty;
        uint8 reserved;
    };

    const char* name;

    uint32 ino;
    struct list_head dev_entry;
};

extern const struct file_operations devfs_file_fops;
extern const struct fs_operations devfs_filesystem_ops;
extern const struct file_system devfs_fs;

/**
 * Initialize the devfs filesystem.
 * @param mp: The mountpoint devfs to mount in.
 * @return: 0 on success, -1 on error.
 */
int devfs_init(struct mountpoint *mp);

/**
 * Add a device to the devfs filesystem.
 * @param device: The device to add.
 */
void devfs_add_device(struct devfs_device *device);

/**
 * Get a device by its path.
 * @param path: The path to the device.
 * @return: A pointer to the device if found, NULL otherwise.
 */
struct devfs_device *devfs_get_by_path(path_t path);

extern struct devfs_device stdin, stdout, stderr;

#endif // __DEVFS_H__