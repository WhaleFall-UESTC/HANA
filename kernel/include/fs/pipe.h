#ifndef __PIPE_H__
#define __PIPE_H__

#include <common.h>
#include <tools/kfifo.h>
#include <locking/spinlock.h>
#include <fs/fs.h>

struct pipe {
	struct kfifo* kfifo;
	spinlock_t* lock;
	int ref;
};

#define PIPE_KFIFO_SIZE_DEFAULT 4096

/**
 * Initialize a pipe with the given read and write file structs.
 * @param rfile: The file struct for reading from the pipe.
 * @param wfile: The file struct for writing to the pipe.
 * @return: 0 on success, -1 on error.
 */
int pipe_init(struct file* rfile, struct file* wfile);

/**
 * Free a pipe structure.
 * @param pipe: The pipe structure to free.
 */
void pipe_free(struct pipe* pipe);

extern const struct file_operations pipe_fileops;

#endif // __PIPE_H__