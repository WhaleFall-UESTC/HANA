#include <fs/pipe.h>
#include <fs/file.h>

#include <mm/mm.h>

int pipe_init(struct file* rfile, struct file* wfile) {
	KCALLOC(struct pipe, rpipe, 1);
	KCALLOC(struct pipe, wpipe, 1);
	KCALLOC(spinlock_t, lock, 1);
	struct kfifo* fifo;

	fifo = kfifo_alloc(PIPE_KFIFO_SIZE_DEFAULT, lock);
	if(fifo == NULL) {
		error("Alloc kfifo error.");
		goto err;
	}

	rpipe->kfifo = wpipe->kfifo = fifo;
	rpipe->lock = wpipe->lock = lock;
	rpipe->rw = PIPE_END_READ;
	wpipe->rw = PIPE_END_WRITE;

	spinlock_init(lock, "pipe");

	rfile->f_op = wfile->f_op = &pipe_fileops;
	rfile->f_private = (void*)rpipe;
	wfile->f_private = (void*)wpipe;

	return 0;
err:
	kfree(rpipe);
	kfree(wpipe);
	kfree(lock);
	return -1;
}

void pipe_free(struct pipe* pipe) {
	if(pipe->lock)
		kfree(pipe->lock);
	if(pipe->kfifo)
		kfifo_free(pipe->kfifo);
	kfree(pipe);
}

static ssize_t pipe_read(struct file* file, char * buffer, size_t size, off_t * offset) {
	struct pipe* pipe = (struct pipe*)file->f_private;
	uint32 rcnt = 0;

	assert(pipe != NULL);
	if(pipe->rw != PIPE_END_READ) {
		error("Not the end for pipe read.");
		return -1;
	}

	if(pipe->kfifo == NULL || pipe->lock == NULL) {
		error("Using partly closed pipe.");
		return -1;
	}

	rcnt = kfifo_get(pipe->kfifo, (void*)buffer, size);
	*offset += rcnt;

	return rcnt;
}

static ssize_t pipe_write(struct file* file, const char * buffer, size_t size, off_t * offset) {
	struct pipe* pipe = (struct pipe*)file->f_private;
	uint32 rcnt = 0;

	assert(pipe != NULL);
	if(pipe->rw != PIPE_END_WRITE) {
		error("Not the end for pipe write.");
		return -1;
	}

	if(pipe->kfifo == NULL || pipe->lock == NULL) {
		error("Using partly closed pipe.");
		return -1;
	}

	rcnt = kfifo_put(pipe->kfifo, (void*)buffer, size);
	*offset += rcnt;

	return rcnt;
}

static int pipe_close(struct file *file) {
	struct pipe* pipe = (struct pipe*)file->f_private;

	assert(pipe != NULL);

	pipe_free(pipe);
	file->f_private = NULL;
	file->f_op = NULL;

	return 0;
}

const struct file_operations pipe_fileops = {
	.read = pipe_read,
	.write = pipe_write,
	.close = pipe_close
};