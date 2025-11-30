#ifndef __DCACHE_H__
#define __DCACHE_H__

#include <common.h>
#include <fs/fs.h>
#include <fs/path.h>
#include <tools/list.h>

#define MAX_FILENAME_LEN 128

struct dentry {
    char d_name[MAX_FILENAME_LEN]; // name of the directory entry
    struct inode *d_inode; // pointer to the inode structure
    struct dentry *d_parent; // pointer to the parent directory entry
    struct list_head d_child; // child entry of parent list
    struct list_head d_subdirs; // list head of child dir entries
};

struct dentry * dentry_alloc(const char *name, int length, struct inode *inode, struct dentry *parent);

void dentry_init(struct dentry *dent, const char *name, int length, struct inode *inode, struct dentry *parent);

#endif // __DCACHE_H__