#ifndef __MOUNTPOINT_H__
#define __MOUNTPOINT_H__

#include <fs/fs.h>
#include <fs/file.h>
#include <fs/devfs/devfs.h>
#include <tools/list.h>

struct mountpoint
{
    const char *mountpoint;
    const struct file_system *fs;
    const struct blkdev *blkdev;
    const struct devfs_device *device;
    struct list_head mp_entry;
    void* private;
};

struct mountpoint* mountpoint_find(const char *path);
void mountpoint_add(struct mountpoint *mp);
void mountpoint_remove(const char *mountpoint);

extern struct list_head mp_listhead;

#define vfs_for_each_mp(mp_ptr) \
    list_for_each_entry(mp_ptr, &mp_listhead, mp_entry)

#define vfs_for_each_mp_safe(mp_ptr, next_ptr) \
    list_for_each_entry_safe(mp_ptr, next_ptr, &mp_listhead, mp_entry)

#endif // __MOUNTPOINT_H__