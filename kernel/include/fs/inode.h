#ifndef __INODE_H__
#define __INODE_H__

#include <common.h>
#include <locking/spinlock.h>
#include <locking/atomic.h>
#include <tools/list.h>

struct inode_operations;
struct mountpoint;
struct file_operations;
struct inode
{
    umode_t i_mode; // file mode
    off_t i_size;   // file size

    const struct inode_operations * i_ops; // inode operations

    // We should save file attributes for the inode in case of fast open
    const struct file_operations * i_fops;
    void* i_fprivate;

    uint32 i_ino; // inode number

    spinlock_t i_lock; // inode lock
    atomic_define(uint32) i_refcount; // reference count

    time_t i_atime; // last access time
    time_t i_mtime; // last modification time
    time_t i_ctime; // last status change time

    struct mountpoint * i_mp; // mountpoint

    union {
        struct device * i_device;
        char * i_link;
        void * i_private;
    };
};

void inode_init(struct inode* inode, umode_t mode, off_t i_size, uint32 ino, struct mountpoint* mp);

// struct inode_operations
// {
// int (*readlink) (struct dentry *, char __user *,int);
// int (*rename)(struct inode *, path_t,
//               struct inode *, path_t, unsigned int);
// int (*setattr)(path_t, struct iattr *);
// ssize_t (*listxattr)(path_t, char *, size_t);
// };

struct inode_operations {
    /**
     * Lookup a dir entry in dir
     */
    void (*lookup)(struct inode *, struct dentry *, unsigned int);
    const char * (*get_link) (struct dentry *, struct inode *);
    void *(*private_clone)(struct inode *);
    // int (*permission) (struct inode *, int);

    // 	int (*readlink) (struct dentry *, char __user *,int);

    // 	int (*link) (struct dentry *,struct inode *,struct dentry *);
    // 	int (*unlink) (struct inode *,struct dentry *);
    // 	int (*symlink) (struct inode *,struct dentry *,const char *);
    // 	int (*mkdir) (struct inode *,struct dentry *,umode_t);
    // 	int (*rmdir) (struct inode *,struct dentry *);
    // int (*mknod) (struct inode *,struct dentry *,umode_t,devid_t);
    // 	int (*rename) (struct inode *, struct dentry *,
    // 			struct inode *, struct dentry *, unsigned int);
    //     int (*setattr) (struct dentry *, struct iattr *);
    //     int (*getattr) (const path_t, struct kstat *, uint32, unsigned int);
    //     ssize_t (*listxattr) (struct dentry *, char *, size_t);
};

#endif // __INODE_H__