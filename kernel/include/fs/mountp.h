#ifndef __MOUNTPOINT_H__
#define __MOUNTPOINT_H__

#include <fs/fs.h>
#include <fs/file.h>
#include <fs/devfs/devfs.h>
#include <tools/list.h>
#include <locking/spinlock.h>

struct mountpoint
{
    const char *mountpoint;
    const struct file_system *fs;
    const struct blkdev *blkdev;
    const struct devfs_device *device;
    struct list_head mp_entry;
    void* private;
};

/**
 * Find a mountpoint by its path.
 * @param path: The path to search for.
 * @return: Pointer to the mountpoint if found, NULL otherwise.
 */
struct mountpoint* mountpoint_find(const char *path);

/**
 * Add a mountpoint to the list.
 * @param mp: Pointer to the mountpoint to add.
 */
void mountpoint_add(struct mountpoint *mp);

/**
 * Remove a mountpoint from the list.
 * @param mountpoint: The path of the mountpoint to remove.
 */
void mountpoint_remove(const char *mountpoint);

extern struct list_head mp_listhead;
extern spinlock_t mplst_lock;

#define vfs_for_each_mp(mp_ptr) \
    list_for_each_entry(mp_ptr, &mp_listhead, mp_entry)

#define vfs_for_each_mp_safe(mp_ptr, next_ptr) \
    list_for_each_entry_safe(mp_ptr, next_ptr, &mp_listhead, mp_entry)

#define mp_list_iter_next_locked(mpptr) \
    ({ \
        void* __ret; \
        spinlock_acquire(&mplst_lock); \
        __ret = list_iter_next(mpptr, &mp_listhead, mp_entry); \
        spinlock_release(&mplst_lock); \
        __ret; \
    })

#define mp_list_iter_init_locked(mpptr) \
    ({ \
        void* __ret; \
        spinlock_acquire(&mplst_lock); \
        __ret = list_iter_init(mpptr, &mp_listhead, mp_entry); \
        spinlock_release(&mplst_lock); \
        __ret; \
    })

/**
 * Iterate over each mountpoint in the list with a locked iterator.
 */
#define mp_list_for_each_entry_locked(mpptr) \
    for(mp_list_iter_init_locked(mpptr); \
        (mpptr) != NULL; mp_list_iter_next_locked(mpptr))

#endif // __MOUNTPOINT_H__