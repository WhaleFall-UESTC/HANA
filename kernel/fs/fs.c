#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/stat.h>
#include <fs/dirent.h>
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
    assert(mount_p->fs->fs_op != NULL);

    mount_p->blkdev = blkdev;

    return mount_p->fs->fs_op->mount(blkdev, mount_p->mountpoint);
}

static int mountpoint_match_prefix(const char* path, struct mountpoint* mount_p)
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
    int max_len = -1, res = -1;

    for(int i = 0; i < mount_count; i++)
    {
        int len = mountpoint_match_prefix(path, &mount_table[i]) - 1;
        if (len > max_len)
        {
            max_len = len;
            res = i;
        }
    }

    return res;
}

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

    while(i_path < path_len && i_f < MAX_PATH_LEN) {
        while(path[i_path] == '/' && i_path < path_len) i_path++;
        if(path[i_path] == '.' && (i_path + 2 == path_len || (i_path + 2 < path_len && path[i_path + 2] == '/')) && path[i_path + 1] == '.') {
            // "../" or "..\0"
            i_path += 2;
            while(i_f > 0 && full_path[i_f - 1] != '/') i_f--;
            if(i_f > 0) i_f--;
        }
        else if(path[i_path] == '.' && (i_path + 1 == path_len || (i_path + 1 < path_len && path[i_path + 1] == '/'))) {
            // "./" or ".\0"
            i_path ++;
        }
        else {
            // normal path
            while(path[i_path] != '/' && i_path < path_len && i_f < MAX_PATH_LEN) {
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

static int get_absolute_path(const char* path, char* full_path)
{
    if (path == NULL || full_path == NULL)
    {
        error("path or full_path is NULL");
        return -1;
    }

    if(path[0] == '/') {
        strcpy(full_path, path);
    }
    else {
        // convert relative path to full path
        strcpy(full_path, myproc()->cwd);
        int ret = fullpath_connect(path, full_path);
        if (ret < 0)
            return -1;
    }

    return 0;
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
    char full_path[MAX_PATH_LEN];
    struct file* file = NULL;
    struct inode* inode = NULL;
    struct files_struct* fdt = myproc()->fdt;
    struct stat stat;

    debug("open path: %s, flags: %d", path, flags);

    ret = get_absolute_path(path, full_path);
    if (ret < 0)
    {
        error("get absolute path error");
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
    
    file->f_flags = flags;
    debug("file->f_flags: %d", file->f_flags);
    ret = mount_p->fs->fs_op->ifget(mount_p, inode, file);
    if(ret < 0) {
        error("ifget error");
        goto out_inode;
    }

    debug("open file %s, flags: %d", full_path, flags);
    ret = file->f_op->open(file, full_path, flags);
    if(ret != EOK) {
        error("open error, ret: %d", ret);
        goto out_inode;
    }

    // add file to fdt

    fd = fd_alloc(fdt, file);
    if (fd < 0)
    {
        error("alloc fd error");
        goto out_inode;
    }

    // fill inode state info

    ret = mount_p->fs->fs_op->getattr(full_path, &stat);
    if (ret != EOK)
    {
        error("stat error");
        goto out_fd;
    }

    inode->i_ino = stat.st_ino;
    inode->i_mode = stat.st_mode;
    inode->i_size = stat.st_size;
    inode->i_atime = stat.st_atime;
    inode->i_mtime = stat.st_mtime;
    inode->i_ctime = stat.st_ctime;
    strcpy(inode->i_path, full_path);

    /**
     * TODO: add inode cache machanism
     */

    return 0;

out_fd:
    fd_free(fdt, fd);
out_inode:
    kfree(inode);
out_file:
    kfree(file);
out_err:
    return -1;
}


SYSCALL_DEFINE3(read, int, fd, char*, buf, size_t, count) {
    struct file* file;
    struct files_struct* fdt = myproc()->fdt;
    int ret;

    if (fd < 0 || fd >= NR_OPEN)
        return -1;

    file = fdt->fd[fd];
    if (file == NULL)
        return -1;

    ret = file->f_op->read(file, buf, count, &file->f_ops);
    if (ret < 0)
        return -1;

    return count;
}

SYSCALL_DEFINE3(write, int, fd, const char*, buf, size_t, count) {
    struct file* file;
    struct files_struct* fdt = myproc()->fdt;
    int ret;

    if (fd < 0 || fd >= NR_OPEN)
        return -1;

    file = fdt->fd[fd];
    if (file == NULL)
        return -1;

    ret = file->f_op->write(file, buf, count, &file->f_ops);
    if (ret < 0)
        return -1;

    return count;
}

SYSCALL_DEFINE1(close, int, fd) {
    struct file* file;
    struct files_struct* fdt = myproc()->fdt;
    int ret;

    if (fd < 0 || fd >= NR_OPEN)
        return -1;

    file = fdt->fd[fd];
    if (file == NULL)
        return -1;

    ret = file->f_op->close(file);
    if (ret < 0)
        return -1;

    kfree(file->f_inode);
    kfree(file);

    return 0;
}

SYSCALL_DEFINE3(lseek, int, fd, off_t, offset, int, whence) {
    struct file* file;
    struct files_struct* fdt = myproc()->fdt;
    int ret;

    if (fd < 0 || fd >= NR_OPEN)
        return -1;

    file = fdt->fd[fd];
    if (file == NULL)
        return -1;

    ret = file->f_op->llseek(file, offset, whence);
    if (ret < 0)
        return -1;

    return ret;
}

SYSCALL_DEFINE2(stat, const char*, path, struct stat*, buf) {
    struct mountpoint* mount_p;
    int ret, mp_index;
    char full_path[MAX_PATH_LEN];

    ret = get_absolute_path(path, full_path);
    if (ret < 0)
    {
        error("get absolute path error");
        return -1;
    }

    // find mountpoint
    mp_index = mountpoint_find(full_path);
    if (mp_index < 0)
    {
        error("mountpoint not found for path %s", full_path);
        return -1;
    }
    mount_p = &mount_table[mp_index];
    assert(mount_p->fs != NULL);

    ret = mount_p->fs->fs_op->getattr(full_path, buf);
    if(ret < 0) {
        error("stat error");
        return -1;
    }

    return 0;
}

SYSCALL_DEFINE2(fstat, int, fd, struct stat*, buf) {
    struct file* file;
    struct files_struct* fdt = myproc()->fdt;
    struct inode* inode;
    int ret;

    if (fd < 0 || fd >= NR_OPEN)
        return -1;

    file = fdt->fd[fd];
    if (file == NULL)
        return -1;

    inode = file->f_inode;
    ret = inode->i_mp->fs->fs_op->getattr(inode->i_path, buf);
    if (ret < 0)
        return -1;

    return 0;
}

SYSCALL_DEFINE1(unlink, const char*, path) {
    struct mountpoint* mount_p;
    int ret, mp_index;
    char full_path[MAX_PATH_LEN];

    ret = get_absolute_path(path, full_path);
    if (ret < 0)
    {
        error("get absolute path error");
        return -1;
    }

    // find mountpoint
    mp_index = mountpoint_find(full_path);
    if (mp_index < 0)
    {
        error("mountpoint not found for path %s", full_path);
        return -1;
    }
    mount_p = &mount_table[mp_index];
    assert(mount_p->fs != NULL);

    ret = mount_p->fs->fs_op->unlink(full_path);
    if(ret < 0) {
        error("unlink error");
        return -1;
    }

    return 0;
}

SYSCALL_DEFINE3(getdents64, int, fd, struct dirent *, buf, size_t, len) {
    struct file* file;
    struct files_struct* fdt = myproc()->fdt;
    int ret;

    if (fd < 0 || fd >= NR_OPEN)
        return -1;

    file = fdt->fd[fd];
    if (file == NULL)
        return -1;

    ret = file->f_op->getdents64(file, buf, len);
    if (ret < 0)
        return -1;

    return ret;
}

SYSCALL_DEFINE2(mkdir, const char*, path, umode_t, mode) {
    struct mountpoint* mount_p;
    int ret, mp_index;
    char full_path[MAX_PATH_LEN];

    ret = get_absolute_path(path, full_path);
    if (ret < 0)
    {
        error("get absolute path error");
        return -1;
    }

    // find mountpoint
    mp_index = mountpoint_find(full_path);
    if (mp_index < 0)
    {
        error("mountpoint not found for path %s", full_path);
        return -1;
    }
    mount_p = &mount_table[mp_index];
    assert(mount_p->fs != NULL);

    debug("mkdir %s, mode = %d", full_path, mode);
    ret = mount_p->fs->fs_op->mkdir(full_path, mode);
    if(ret < 0) {
        error("mkdir error");
        return -1;
    }

    return 0;
}

SYSCALL_DEFINE1(rmdir, const char*, path) {
    struct mountpoint* mount_p;
    int ret, mp_index;
    char full_path[MAX_PATH_LEN];

    ret = get_absolute_path(path, full_path);
    if (ret < 0)
    {
        error("get absolute path error");
        return -1;
    }

    // find mountpoint
    mp_index = mountpoint_find(full_path);
    if (mp_index < 0)
    {
        error("mountpoint not found for path %s", full_path);
        return -1;
    }
    mount_p = &mount_table[mp_index];
    assert(mount_p->fs != NULL);

    ret = mount_p->fs->fs_op->rmdir(full_path);
    if(ret < 0) {
        error("rmdir error");
        return -1;
    }

    return 0;
}

SYSCALL_DEFINE2(link, const char*, oldpath, const char*, newpath) {
    struct mountpoint* mount_p;
    int ret, mp_index;
    char full_old_path[MAX_PATH_LEN];
    char full_new_path[MAX_PATH_LEN];

    ret = get_absolute_path(oldpath, full_old_path);
    if (ret < 0)
    {
        error("get absolute old path error");
        return -1;
    }

    ret = get_absolute_path(newpath, full_new_path);
    if (ret < 0)
    {
        error("get absolute new path error");
        return -1;
    }

    // find mountpoint
    mp_index = mountpoint_find(full_old_path);
    if (mp_index < 0)
    {
        error("mountpoint not found for path %s", full_old_path);
        return -1;
    }
    mount_p = &mount_table[mp_index];
    assert(mount_p->fs != NULL);

    ret = mount_p->fs->fs_op->link(full_old_path, full_new_path);
    if(ret < 0) {
        error("link error");
        return -1;
    }

    return 0;
}

SYSCALL_DEFINE2(symlink, const char *, target, const char *, linkpath) {
    struct mountpoint* mount_p;
    int ret, mp_index;
    char full_target_path[MAX_PATH_LEN];
    char full_link_path[MAX_PATH_LEN];

    ret = get_absolute_path(target, full_target_path);
    if (ret < 0)
    {
        error("get absolute target path error");
        return -1;
    }

    ret = get_absolute_path(linkpath, full_link_path);
    if (ret < 0)
    {
        error("get absolute link path error");
        return -1;
    }

    // find mountpoint
    mp_index = mountpoint_find(full_target_path);
    if (mp_index < 0)
    {
        error("mountpoint not found for path %s", full_target_path);
        return -1;
    }
    mount_p = &mount_table[mp_index];
    assert(mount_p->fs != NULL);

    ret = mount_p->fs->fs_op->symlink(full_target_path, full_link_path);
    if(ret < 0) {
        error("symlink error");
        return -1;
    }

    return 0;
}