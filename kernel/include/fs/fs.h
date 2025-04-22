#ifndef __FS_H__
#define __FS_H__

#include <common.h>
#include <locking/spinlock.h>

// struct path
// {
//     int p_len;
//     char p_name[0];
// };

#define MAX_PATH_LEN 256
typedef const char* path_t;

struct inode_operations;
struct mountpoint;

struct inode
{
    umode_t i_mode; // file mode
    off_t i_size;   // file size

    const struct inode_operations *i_op; // inode operations

    char i_path[MAX_PATH_LEN]; // path to the file
    uint32 i_ino;              // inode number

    spinlock_t i_lock;   // inode lock
    uint32_t i_refcount; // reference count

    time_t i_atime; // last access time
    time_t i_mtime; // last modification time
    time_t i_ctime; // last status change time

    struct mountpoint *i_mp; // mountpoint
};

struct dentry;

/**
 * TODO: Let the interfaces accept inode and dentry as a parameter rather than path
 * This may lead to a lot of changes in lwext4
 */

// struct inode_operations
// {
    // int (*readlink) (struct dentry *, char __user *,int);
    // int (*rename)(struct inode *, path_t,
    //               struct inode *, path_t, unsigned int);
    // int (*setattr)(path_t, struct iattr *);
    // ssize_t (*listxattr)(path_t, char *, size_t);
// };

// struct inode_operations {
// int (*permission) (struct inode *, int);

// 	int (*readlink) (struct dentry *, char __user *,int);

// 	int (*link) (struct dentry *,struct inode *,struct dentry *);
// 	int (*unlink) (struct inode *,struct dentry *);
// 	int (*symlink) (struct inode *,struct dentry *,const char *);
// 	int (*mkdir) (struct inode *,struct dentry *,umode_t);
// 	int (*rmdir) (struct inode *,struct dentry *);
// int (*mknod) (struct inode *,struct dentry *,umode_t,dev_t);
// 	int (*rename) (struct inode *, struct dentry *,
// 			struct inode *, struct dentry *, unsigned int);
//     int (*setattr) (struct dentry *, struct iattr *);
//     int (*getattr) (const path_t, struct kstat *, uint32, unsigned int);
//     ssize_t (*listxattr) (struct dentry *, char *, size_t);
// };

struct file;

struct fs_operations
{
    int (*mount)(struct blkdev *, const char *);
    // int (*umount)(const char *mount_point);
    // int (*statfs)(const char *mount_point, struct statfs *);
    /**
     * get fs specific inode and file
     * it init ops ptr and private data, but DO NOT alloc inode and file
     */
    int (*ifget)(struct mountpoint *, struct inode *, struct file *);
    int (*link)(path_t, path_t);
    int (*unlink)(path_t);
    int (*symlink)(path_t, path_t);
    int (*mkdir)(path_t, umode_t);
    int (*rmdir)(path_t);
    int (*rename)(path_t, path_t);
    int (*getattr)(const path_t, struct stat *);
};

struct file_system
{
    const char *name;
    const struct fs_operations *fs_op;
};

struct mountpoint
{
    const char *mountpoint;
    struct file_system *fs;
};

#define NR_MOUNT 16
#define MOUNT_ROOT "/"

int mount(const char *blkdev_name, struct mountpoint *mount_p);

extern struct mountpoint mount_table[NR_MOUNT], *root_mp;
extern int mount_count;

#define mount_root(blkname, filesystem)   \
    do                                    \
    {                                     \
        root_mp->mountpoint = MOUNT_ROOT; \
        root_mp->fs = &(filesystem);      \
        mount(blkname, root_mp);          \
    } while (0)
#define mount_addp(blkname, filesystem, mountpoint)         \
    do                                                      \
    {                                                       \
        mount_table[mount_count].mountpoint = (mountpoint); \
        mount_table[mount_count].fs = &(filesystem);        \
        mount(blkname, &mount_table[mount_count]);          \
        mount_count++;                                      \
    } while (0)

#endif // __FS_H__