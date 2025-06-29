#include <io/blk.h>
#include <io/chr.h>
#include <io/device.h>
#include <fs/fs.h>
#include <fs/fcntl.h>
#include <fs/stat.h>
#include <fs/file.h>
#include <fs/mountp.h>
#include <fs/dirent.h>
#include <fs/devfs/devfs.h>
#include <klib.h>
#include <locking/spinlock.h>
#include <mm/mm.h>
#include <init.h>

static struct list_head devfs_list;
static spinlock_t devfs_list_lk;
static uint32 cur_ino = 0;

struct devfs_device stdin, stdout, stderr;

static struct devfs_device *devfs_get_by_name(const char *name)
{
    struct devfs_device *device;

    spinlock_acquire(&devfs_list_lk);
    list_for_each_entry(device, &devfs_list, dev_entry)
    {
        if (strncmp(name, device->name, DEV_NAME_MAX_LEN) == 0)
        {
            spinlock_release(&devfs_list_lk);
            return device;
        }
    }
    spinlock_release(&devfs_list_lk);

    return NULL;
}

/**
 * Check if path is valid devfs path
 * @return 0 if path is directly "/dev", -1 if error, otherwise name position in path
 */
static inline int get_name_pos_from_path(const char *path)
{
    int pos, len, ret;

    if (strncmp(path, "/dev", 4) != 0)
        return -1;

    len = strlen(path);

    if (len == 4)
        return 0;

    if (path[4] != '/')
        return -1;

    for (pos = 4; pos < len && path[pos] == '/'; pos++)
        ;

    ret = pos;
    for (; pos < len; pos++)
        if (path[pos] == '/')
            return -1;

    if (ret == len)
        return 0;

    return ret;
}

struct devfs_device *devfs_get_by_path(path_t path) {
    int pos = get_name_pos_from_path(path);
    if(pos <= 0)
        return NULL;
    return devfs_get_by_name(path + pos);
}

/**
 * Init devfs and related subsystem
 */
int devfs_init(struct mountpoint *mp)
{
    struct chrdev* chrdev;
    struct blkdev* blkdev;
    struct devfs_device* device;
    struct device *dev;
    char blkname[4] = "sda\0";

    mp->mountpoint = strdup("/dev");
    mp->blkdev = NULL;
    mp->device = NULL;
    mp->fs = &devfs_fs;
    mp->private = NULL;

    spinlock_init(&devfs_list_lk, "devfs list lock");
    INIT_LIST_HEAD(devfs_list);

    chrdev = chrdev_get_default_dev();
    assert(chrdev != NULL);

    tty_init(&stdin.tty, chrdev, "stdin", 0);
    tty_init(&stdout.tty, chrdev, "stdout", TTY_BUF_SIZE_DEFAULT);
    tty_init(&stderr.tty, chrdev, "stderr", 0);

    stdin.file_type = FT_CHRDEV;
    stdin.name = stdin.tty.name;
    devfs_add_device(&stdin);

    stdout.file_type = FT_CHRDEV;
    stdout.name = stdout.tty.name;
    devfs_add_device(&stdout);

    stderr.file_type = FT_CHRDEV;
    stderr.name = stderr.tty.name;
    devfs_add_device(&stderr);
    
    device_list_for_each_entry_locked(dev) {
        if(dev->type == DEVICE_TYPE_BLOCK) {
            device = kcalloc(1, sizeof(*device));
            assert(device != NULL);

            blkdev = container_of(dev, struct blkdev, dev);
            assert(blkname[2] != 'z')
            block_init(&device->disk, blkdev, blkname);
            device->file_type = FT_BLKDEV;
            device->name = device->disk.name;
            devfs_add_device(device);

            blkname[2] ++;
        }
    }

    return 0;
}

void devfs_add_device(struct devfs_device *device)
{
    device->ino = cur_ino++;
    spinlock_acquire(&devfs_list_lk);
    list_insert(&devfs_list, &device->dev_entry);
    spinlock_release(&devfs_list_lk);
}

static int devfs_fs_ifget(struct mountpoint *mp, struct inode *inode, struct file *file)
{
    file->f_op = &devfs_file_fops;
    file->f_inode = inode;
    inode->i_mp = mp;
    return 0;
}

static off_t devfs_llseek(struct file *file, off_t offset, int whence)
{
    struct devfs_device *device = (struct devfs_device *)file->f_private;
    assert(device != NULL);

    if (device->file_type == FT_DIR)
    {
        error("cannot seek in dir");
        return -1;
    }
    else if (device->file_type == FT_BLKDEV)
    {
        return block_llseek(&device->disk, offset, whence);
    }
    else if (device->file_type == FT_CHRDEV)
    {
        return tty_llseek(&device->tty, offset, whence);
    }

    error("Invalid device type");
    return -1;
}

static ssize_t devfs_read(struct file *file, char *buffer, size_t size, off_t *offset)
{
    struct devfs_device *device = (struct devfs_device *)file->f_private;
    assert(device != NULL);

    if (device->file_type == FT_DIR)
    {
        error("cannot read in dir");
        return -1;
    }
    else if (device->file_type == FT_BLKDEV)
    {
        return block_read(&device->disk, buffer, size, offset);
    }
    else if (device->file_type == FT_CHRDEV)
    {
        return tty_read(&device->tty, buffer, size, offset);
    }

    error("Invalid device type");
    return -1;
}

static ssize_t devfs_write(struct file *file, const char *buffer, size_t size, off_t *offset)
{
    struct devfs_device *device = (struct devfs_device *)file->f_private;
    assert(device != NULL);

    if (device->file_type == FT_DIR)
    {
        error("cannot write in dir");
        return -1;
    }
    else if (device->file_type == FT_BLKDEV)
    {
        return block_write(&device->disk, buffer, size, offset);
    }
    else if (device->file_type == FT_CHRDEV)
    {
        return tty_write(&device->tty, buffer, size, offset);
    }

    error("Invalid device type");
    return -1;
}

static int devfs_openat(struct file *file, path_t path, int flags, umode_t mode)
{
    struct devfs_device *device;
    int pos;

    if (flags & O_CREAT || flags & O_APPEND)
    {
        error("Flag not supported");
        return -1;
        /**
         * TODO: More unsupported flags to add
         */
    }

    pos = get_name_pos_from_path(path);

    if (pos < 0)
    {
        error("Invalid path");
        return -1;
    }
    else if (pos == 0)
    {
        device = kcalloc(1, sizeof(*device));
        device->file_type = FT_DIR;
        file->f_inode->i_mode = mode & S_IFDIR;
    }
    else
    {
        device = devfs_get_by_name(path + pos);

        if (device == NULL)
        {
            error("devfs not found.");
            return -1;
        }

        if (device->file_type == FT_BLKDEV)
            file->f_inode->i_mode = mode & S_IFBLK;
        else if (device->file_type == FT_CHRDEV)
            file->f_inode->i_mode = mode & S_IFCHR;
        else
        {
            error("Invalid data type.");
            return -1;
        }
    }

    file->f_private = (void *)device;

    return 0;
}

static int devfs_close(struct file *file)
{
    struct devfs_device *device = (struct devfs_device *)file->f_private;
    assert(device != NULL);

    if (device->file_type == FT_DIR)
    {
        kfree(device);
    }
    file->f_private = NULL;
    return 0;
}

static int devfs_getattr(path_t path, struct stat *stat)
{
    struct devfs_device *device;

    assert(stat != NULL);

    int pos;
    pos = get_name_pos_from_path(path);
    if (pos < 0)
    {
        error("Invalid path");
        return -1;
    }
    else if (pos == 0)
    {
        stat->st_dev = 0;
        stat->st_ino = 1;
        stat->st_mode = S_IFDIR;
        stat->st_nlink = 1;
        stat->st_uid = 0;
        stat->st_gid = 0;
        stat->st_rdev = 0;
        stat->st_size = 4096;
        stat->st_blksize = 512;
        stat->st_blocks = (uint64)8;
        stat->st_atime = 0;
        stat->st_atime_nsec = 0;
        stat->st_ctime = 0;
        stat->st_ctime_nsec = 0;
        stat->st_mtime = 0;
        stat->st_mtime_nsec = 0;
    }
    else
    {
        device = devfs_get_by_name(path + pos);

        if (device == NULL)
        {
            error("devfs not found.");
            return -1;
        }

        if (device->file_type == FT_BLKDEV)
        {
            stat->st_dev = device->disk.blkdev->dev.devid;
            stat->st_ino = device->ino;
            stat->st_mode = S_IFBLK;
            stat->st_nlink = 1;
            stat->st_uid = 0;
            stat->st_gid = 0;
            stat->st_rdev = 0;
            stat->st_size = device->disk.blkdev->size;
            stat->st_blksize = device->disk.blkdev->sector_size;
            stat->st_blocks = device->disk.blkdev->size / device->disk.blkdev->sector_size;
            stat->st_atime = 0;
            stat->st_atime_nsec = 0;
            stat->st_ctime = 0;
            stat->st_ctime_nsec = 0;
            stat->st_mtime = 0;
            stat->st_mtime_nsec = 0;
        }
        else if (device->file_type == FT_CHRDEV)
        {
            stat->st_dev = device->tty.chrdev->dev.devid;
            stat->st_ino = device->ino;
            stat->st_mode = S_IFCHR;
            stat->st_nlink = 1;
            stat->st_uid = 0;
            stat->st_gid = 0;
            stat->st_rdev = 0;
            stat->st_size = 0;
            stat->st_blksize = 0;
            stat->st_blocks = 0;
            stat->st_atime = 0;
            stat->st_atime_nsec = 0;
            stat->st_ctime = 0;
            stat->st_ctime_nsec = 0;
            stat->st_mtime = 0;
            stat->st_mtime_nsec = 0;
        }
        else
        {
            error("Invalid data type.");
            return -1;
        }
    }

    /**
     * TODO: add st_xtime_nsecs and st_dev
     */

    return 0;
}

static int devfs_getdents64(struct file *file, struct dirent *buf, size_t len)
{
    struct devfs_device *device = (struct devfs_device *)file->f_private;
    assert(device != NULL);
    size_t size, name_length;
    int ret = 0;

    if (device->file_type != FT_DIR)
    {
        error("Not a directory");
        return -1;
    }

    spinlock_acquire(&devfs_list_lk);
    list_for_each_entry(device, &devfs_list, dev_entry)
    {
        name_length = strlen(device->name);
        size = sizeof(struct dirent) + name_length + 1;

        if (len < size)
            break;

        buf->d_ino = device->ino;
        buf->d_off = 0;
        buf->d_type = fs_ftype_to_dtype(device->file_type);

        memcpy(buf->d_name, device->name, name_length);
        buf->d_reclen = size;

        buf = (struct dirent *)((uint64)buf + size);
        len -= size;
        ret += size;
    }
    spinlock_release(&devfs_list_lk);

    return ret;
}

const struct file_operations devfs_file_fops = {
    .llseek = devfs_llseek,
    .read = devfs_read,
    .write = devfs_write,
    .openat = devfs_openat,
    .close = devfs_close,
    .getdents64 = devfs_getdents64,
};

const struct fs_operations devfs_filesystem_ops = {
    .ifget = devfs_fs_ifget,
    .getattr = devfs_getattr,
};

const struct file_system devfs_fs = {
    .name = "devfs",
    .fs_op = &devfs_filesystem_ops,
};