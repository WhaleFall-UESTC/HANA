#ifndef __PIPE_H__
#define __PIPE_H__

#include <common.h>
#include <tools/kfifo.h>
#include <locking/spinlock.h>
#include <fs/fs.h>

struct pipe {
	struct kfifo* kfifo;
	spinlock_t* lock;
#define PIPE_END_READ 0
#define PIPE_END_WRITE 1
	uint8 rw;
};

#define PIPE_KFIFO_SIZE_DEFAULT 4096

int pipe_init(struct file* rfile, struct file* wfile);
void pipe_free(struct pipe* pipe);

extern const struct file_operations pipe_fileops;

#endif // __PIPE_H__