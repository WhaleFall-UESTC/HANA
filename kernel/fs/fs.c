#include <fs/fs.h>
#include <fs/file.h>
#include <fs/dcache.h>
#include <fs/ext4/ext4.h>
#include <fs/ext4/ext4_blk.h>

#include <io/blk.h>
#include <proc/proc.h>
#include <syscall.h>
#include <debug.h>

struct mountpoint root_mp, mount_table[NR_MOUNT];
int mount_count = 0;

/************************ Export and helper functions ************************/

int mount(const char *blkdev_name, struct mountpoint *mount_p)
{
    struct blkdev* blkdev = blkdev_get_by_name(blkdev_name);
    if (blkdev == NULL)
    {
        error("block device %s not found", blkdev_name);
        return -1;
    }

    assert(mount_p != NULL);
    assert(mount_p->fs != NULL);
    assert(mount_p->fs->mount != NULL);

    return mount_p->fs->mount(blkdev, mount_p->mountpoint);
}

/************************ Syscalls for filesystems *************************/

SYSCALL_DEFINE2(open, const char*, filename, unsigned int, flags) {
    
}
