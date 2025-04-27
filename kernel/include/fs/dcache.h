#ifndef __DCACHE_H__
#define __DCACHE_H__

#include <common.h>
#include <fs/fs.h>
#include <tools/list.h>

struct dentry {
    char *d_name; // name of the directory entry
    struct inode *d_inode; // pointer to the inode structure
    struct dentry *d_parent; // pointer to the parent directory entry
    struct list_head d_child; // child of parent list
    struct list_head d_subdirs; // our children
};

#endif // __DCACHE_H__