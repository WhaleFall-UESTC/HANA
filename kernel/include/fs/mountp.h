#ifndef __MOUNTPOINT_H__
#define __MOUNTPOINT_H__

#include <fs/fs.h>
#include <fs/file.h>
#include <fs/devfs/devfs.h>

#define NR_MOUNT 16

struct mountpoint
{
    const char *mountpoint;
    const struct file_system *fs;
    const struct blkdev *blkdev;
    const struct devfs_device *device;
    void* private;
};

struct mountpoint* mountpoint_find(const char *path, int start);
void mountpoint_add(struct mountpoint *mp);
void mountpoint_remove(const char *mountpoint);

extern struct mountpoint mount_table[NR_MOUNT];
extern int mount_count;

#endif // __MOUNTPOINT_H__