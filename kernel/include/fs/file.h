#ifndef __FILE_H__
#define __FILE_H__

#include <common.h>
#include <fs/fs.h>
#include <fs/dirent.h>
#include <klib.h>
#include <locking/atomic.h>

struct file_operations;

struct file
{
	unsigned int f_flags;
	// fmode_t f_mode;
	const struct file_operations *f_op;
	struct inode *f_inode;

	char f_path[MAX_PATH_LEN];

	off_t fpos;

	void *f_private;

	/**
	 * Reference count of a struct file
	 * f_ref = 0 will lead to free it
	 */
	atomic_define(int) f_ref;
};

struct file_operations
{
	off_t (*llseek)(struct file *, off_t, int);
	ssize_t (*read)(struct file *, char *, size_t, off_t *);
	ssize_t (*write)(struct file *, const char *, size_t, off_t *);
	int (*openat)(struct file *, path_t, int, umode_t);
	int (*close)(struct file *);
	int (*getdents64)(struct file *, struct dirent *, size_t);
	int (*truncate)(struct file*, off_t length);
	// int (*flush) (struct file *, fl_owner_t id);
	// int (*fsync) (struct file *, off_t, off_t, int datasync);
};

#define NR_OPEN 1024

struct files_struct
{
	struct file *fd[NR_OPEN];
	unsigned int next_fd; // we maintain a next_fd after any fd inc/dec
	unsigned int nr_avail_fd;
	spinlock_t fdt_lock; // lock for fd table
};

/**
 * Initialize file struct for stdin, stdout, stderr.
 */
void iofd_init();

/**
 * Initialize the file descriptor table.
 * @param files The files_struct given.
 * @param name The name given to lock.
 * @return 0 on success, -1 on error.
 */
void fdt_init(struct files_struct *files, char* name);

/**
 * Duplicate a files_struct, dup all struct file in fdt.
 * @param fdt The files_struct to be duplicated.
 * @return A new files_struct on success, NULL on error.
 */
struct files_struct* fdt_dup(struct files_struct *fdt);

/**
 * Free all struct file in a fdt and make it unavailable
 * @param fdt The files_struct to be freed
 */
void fdt_freeall(struct files_struct *fdt);

/**
 * Allocate a file descriptor in given files_struct.
 * @param fdt The files_struct given.
 * @param file The struct file to be filled in fdt_i.
 * @return The allocated file descriptor, or -1 on error.
 */
fd_t fd_alloc(struct files_struct *fdt, struct file *file);

/**
 * Free a file descriptor in given files_struct.
 * @param fdt The files_struct given.
 * @param fd The file descriptor to free.
 */
void fd_free(struct files_struct *fdt, fd_t fd);

/**
 * Clone a struct file from a old fd to a new one.
 * If new_fd is set to -1, return the lowest-numbered fd unused.
 * @param fdt The files_struct given.
 * @param old_fd The old file descriptor.
 * @param new_fd The new file descriptor.
 * @return new_fd on success, -1 on error.
 */
int fd_clone(struct files_struct *fdt, fd_t old_fd, fd_t new_fd);

/**
 * Get a struct file in fd.
 * @param fdt The files_struct given.
 * @param fd The file descriptor.
 * @return NULL on error, otherwise success.
 */
struct file* fd_get(struct files_struct *fdt, fd_t fd);

/**
 * Initialize a struct file.
 * @param file The struct file to be initialized.
 * @param f_op The file operations to be set.
 * @param path The path of the file, can be NULL.
 * @param flags The flags of the file, can be 0.
 * @param private The private data of the file, can be 0.
 */
void file_init(struct file *file, const struct file_operations *f_op, const char *path, unsigned int flags, void* private);

/**
 * Increase the reference count of a struct file.
 */
void file_get(struct file *file);

/**
 * Decrease the reference count of a struct file.
 * If the reference count reaches 0, close file and free the struct file.
 * If the file has an inode, free it too.
 * @return: reference count of the struct file on success, -1 on error.
 */
int file_put(struct file *file);

int file_refcnt(struct file *file);

#endif // __FILE_H__