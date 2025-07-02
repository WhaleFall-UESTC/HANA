#include <fs/pipe.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <mm/mm.h>

int pipe_init(struct file* rfile, struct file* wfile) {
	KCALLOC(struct pipe, pipe, 1);
	KCALLOC(spinlock_t, lock, 1);
	struct kfifo* fifo;

	fifo = kfifo_alloc(PIPE_KFIFO_SIZE_DEFAULT, lock);
	if(fifo == NULL) {
		error("Alloc kfifo error.");
		goto err;
	}

	pipe->kfifo = fifo;
	pipe->lock = lock;
	pipe->ref = 2;

	spinlock_init(lock, "pipe");

	file_init(rfile, &pipe_fileops, NULL, O_RDONLY, (void*)pipe);
	file_init(wfile, &pipe_fileops, NULL, O_WRONLY, (void*)pipe);

	return 0;
err:
	kfree(pipe);
	kfree(lock);
	return -1;
}

void pipe_free(struct pipe* pipe) {
	if(pipe && !--pipe->ref) {
		if(pipe->kfifo) {
			kfifo_free(pipe->kfifo);
			pipe->kfifo = NULL;
		}
		if(pipe->lock) {
			kfree(pipe->lock);
			pipe->lock = NULL;
		}
		kfree(pipe);
	}
}

/**
 * Read data from a pipe.
 * @param file: The file structure containing the pipe.
 * @param buffer: The buffer to read data into.
 * @param size: The number of bytes to read.
 * @param offset: The current offset in the file.
 * @return: The number of bytes read, or -1 on error.
 */
static ssize_t pipe_read(struct file* file, char * buffer, size_t size, off_t * offset) {
	struct pipe* pipe = (struct pipe*)file->f_private;
	uint32 rcnt = 0;

	assert(pipe != NULL);

	if(pipe->kfifo == NULL || pipe->lock == NULL) {
		error("Pipe lack fifo or lock.");
		return -1;
	}

	rcnt = kfifo_get(pipe->kfifo, (void*)buffer, size);
	*offset += rcnt;

	return rcnt;
}

/**
 * Write data to a pipe.
 * @param file: The file structure containing the pipe.
 * @param buffer: The buffer containing data to write.
 * @param size: The number of bytes to write.
 * @param offset: The current offset in the file.
 * @return: The number of bytes written, or -1 on error.
 */
static ssize_t pipe_write(struct file* file, const char * buffer, size_t size, off_t * offset) {
	struct pipe* pipe = (struct pipe*)file->f_private;
	uint32 rcnt = 0;

	assert(pipe != NULL);

	if(pipe->kfifo == NULL || pipe->lock == NULL) {
		error("Pipe lack fifo or lock.");
		return -1;
	}

	rcnt = kfifo_put(pipe->kfifo, (void*)buffer, size);
	*offset += rcnt;

	return rcnt;
}


/**
 * Close a pipe file.
 * @param file: The file structure containing the pipe.
 * @return: 0 on success, -1 on error.
 */
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