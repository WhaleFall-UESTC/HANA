#include <fs/fs.h>
#include <fs/file.h>
#include <fs/dcache.h>
#include <fs/ext4/ext4.h>
#include <fs/ext4/ext4_blk.h>

#include <io/blk.h>
#include <proc/proc.h>
#include <syscall.h>
#include <debug.h>

struct mountpoint mount_table[NR_MOUNT], *root_mp = &mount_table[0];
int mount_count = 1;

/************************ Export and Helper functions ************************/

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

static int mountpoint_maxprefix(const char* path, struct mountpoint* mount_p)
{
    int mp_len = strlen(mount_p->mountpoint);
    int path_len = strlen(path);
    int end_index = 0;

    while(end_index < mp_len && end_index < path_len && 
          mount_p->mountpoint[end_index] == path[end_index])
    {
        end_index++;
    }

    return end_index;
}

static int mountpoint_find(const char* path)
{
    int max_len = 0, res = -1;

    for(int i = 0; i < mount_count; i++)
    {
        int len = mountpoint_maxprefix(path, &mount_table[i]) - 1;
        if (len > max_len)
        {
            max_len = len;
            res = i;
        }
    }

    return res;
}

#define MAX_FULLPATH_LEN 256
/**
 * Convert relative path to full path, remove all "." and ".."
 * use this only when path is NOT started with "/"
 * @path: path to convert
 * @full_path: buffer to store full path, should be initialized by cwd
 * @return: length of full path on success, -1 on error
 */
static int fullpath_connect(const char* path, char* full_path)
{
    int path_len = strlen(path);
    int i_path = 0, i_f = strlen(full_path);

    if(path[0] == '/') return -1;

    if(full_path[i_f - 1] == '/') i_f --;

    while(i_path < path_len && i_f < MAX_FULLPATH_LEN) {
        while(path[i_path] == '/' && i_path < path_len) i_path++;
        if(path[i_path] == '.' && (i_path + 2 == path_len || i_path + 2 < path_len && path[i_path + 2] == '/') && path[i_path + 1] == '.') {
            // "../" or "..\0"
            i_path += 2;
            while(i_f > 0 && full_path[i_f - 1] != '/') i_f--;
            if(i_f > 0) i_f--;
        }
        else if(path[i_path] == '.' && (i_path + 1 == path_len || i_path + 1 < path_len && path[i_path + 1] == '/')) {
            // "./" or ".\0"
            i_path ++;
        }
        else {
            // normal path
            while(path[i_path] != '/' && i_path < path_len && i_f < MAX_FULLPATH_LEN) {
                full_path[i_f++] = path[i_path++];
            }
        }
    }

    if(i_path < path_len) {
        error("path too long");
        return -1;
    }

    full_path[i_f] = '\0';
    return i_f;
}

/************************ Syscalls for filesystems *************************/

/**
 * open syscall
 * We currently alloc and free BOTH inode and file in open and close
 * @path: path to open
 * @flags: open flags
 * @return: fd on success, -1 on error
 */
SYSCALL_DEFINE2(open, const char*, path, unsigned int, flags) {
    fd_t fd;
    struct mountpoint* mount_p = NULL;
    int ret, mp_index;
    char full_path[MAX_FULLPATH_LEN];
    struct file* file = NULL;
    struct inode* inode = NULL;

    if (path == NULL)
    {
        error("path is NULL");
        return -1;
    }

    if(path[0] == '/') {
        strcpy(full_path, path);
    }
    else {
        // convert relative path to full path
        strcpy(full_path, myproc()->cwd);
        ret = fullpath_connect(path, full_path);
        if (ret < 0)
            return -1;
    }

    // find mountpoint
    mp_index = mountpoint_find(full_path);
    if (mp_index < 0)
    {
        error("mountpoint not found for path %s", full_path);
        goto out_err;
    }
    mount_p = &mount_table[mp_index];
    assert(mount_p->fs != NULL);

    file = kcalloc(1, sizeof(struct file));
    if (file == NULL)
    {
        error("alloc file error");
        goto out_err;
    }

    inode = kcalloc(1, sizeof(struct inode));
    if (inode == NULL)
    {
        error("alloc inode error");
        goto out_file;
    }
    
    ret = mount_p->fs->ifget(inode, file);
    if(ret < 0) {
        error("ifget error");
        goto out_inode;
    }

    ret = file->f_op->open(file, full_path, flags);
    if(ret != EOK) {
        error("open error, ret: %d", ret);
        goto out_inode;
    }

    // add file and inode flags

    // add file to fdt

    return 0;
out_inode:
    kfree(inode);
out_file:
    kfree(file);
out_err:
    return -1;
}
