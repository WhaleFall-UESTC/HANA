#ifndef __FILE_H__
#define __FILE_H__

#include <common.h>
#include <fs/fs.h>

struct file_operations;

struct file {
    unsigned int f_flags;
    const struct file_operations *f_op;
    struct inode *f_inode;

    off_t f_ops;

    void* f_private;
};

struct file_operations {
	off_t (*llseek) (struct file *, off_t, int);
	ssize_t (*read) (struct file *, char *, size_t, off_t *);
	ssize_t (*write) (struct file *, const char *, size_t, off_t *);
	int (*open) (struct file *, struct path *, uint32);
	int (*close) (struct file *);
	// int (*flush) (struct file *, fl_owner_t id);
	// int (*fsync) (struct file *, off_t, off_t, int datasync);
};

#endif // __FILE_H__