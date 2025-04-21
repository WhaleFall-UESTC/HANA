#ifndef __FILE_H__
#define __FILE_H__

#include <common.h>
#include <fs/fs.h>
#include <klib.h>
#include <locking/spinlock.h>

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

typedef unsigned int fd_t;

#define NR_OPEN 1024

struct files_struct {
	struct file *fd[NR_OPEN];
	unsigned int next_fd; // we maintain a next_fd after any fd inc/dec
	unsigned int nr_avail_fd;
	spinlock_t fdt_lock; // lock for fd table
};


/**
 * Initialize the file descriptor table.
 * @param files The files_struct given.
 * @return 0 on success, -1 on error.
 */
void fdt_init(struct files_struct *files);

/**
 * Allocate a file descriptor for the current process.
 * @param fdt The files_struct of the current process.
 * @return The allocated file descriptor, or -1 on error.
 */
fd_t fd_alloc(struct files_struct *fdt);

#endif // __FILE_H__