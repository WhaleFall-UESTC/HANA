#include <fs/fs.h>
#include <fs/file.h>
#include <fs/dcache.h>
#include <klib.h>

struct dentry * dentry_alloc(const char *name, int length, struct inode *inode, struct dentry *parent) {
	KALLOC(struct dentry, dent);
	dentry_init(dent, name, length, inode, parent);
	return dent;
}

void dentry_init(struct dentry *dent, const char *name, int length, struct inode *inode, struct dentry *parent) {
	strncpy(dent->d_name, name, length);
	dent->d_inode = inode;
	dent->d_parent = parent;
	INIT_LIST_HEAD(dent->d_subdirs);
}

