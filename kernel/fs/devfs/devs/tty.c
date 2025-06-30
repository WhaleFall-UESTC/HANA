#include <fs/devfs/devs/tty.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <mm/mm.h>

/**
 * Flush the TTY output buffer to the character device
 * @param tty: The TTY structure to flush
 */
static inline void tty_flush(struct tty* tty) {
    size_t pos;

    assert(tty->buffer != NULL);

    for(pos = 0; pos < tty->bofs; pos ++)
        tty->chrdev->ops->putchar(tty->chrdev, tty->buffer[pos]);
    
    tty->bofs = 0;
}

ssize_t tty_read(struct tty* tty, char *buffer, size_t size, off_t *offset)
{
    size_t nr_read;
    char c;

    for(nr_read = 0; nr_read < size; nr_read ++) {
        c = tty->chrdev->ops->getchar(tty->chrdev);

        if(c == -1) {
            error("tty read interrupted");
            break;
        }

        buffer[nr_read] = c;
    }

    *offset += nr_read;
    return nr_read;
}

ssize_t tty_write(struct tty* tty, const char *buffer, size_t size, off_t *offset)
{
    size_t nr_write;
    char c;

    for(nr_write = 0; nr_write < size; nr_write ++) {
        c = buffer[nr_write];

        if(tty->buffer) {
            tty->buffer[tty->bofs ++] = c;
            if(c == '\n' || tty->bofs == tty->bufsize)
                tty_flush(tty);
        }
        else tty->chrdev->ops->putchar(tty->chrdev, c);
    }

    *offset += nr_write;
    return nr_write;
}

off_t tty_llseek(struct tty* tty, off_t offset, int whence)
{
    error("tty no lseek");
	return -1;
}

void tty_init(struct tty* tty, struct chrdev* chrdev, const char* name, size_t bufsize) {
    assert(tty != NULL);

    tty->chrdev = chrdev;
    strncpy(tty->name, name, MAX_FILENAME_LEN);
    tty->bufsize = bufsize;
    if(bufsize)
        tty->buffer = kalloc(bufsize);
    else tty->buffer = NULL;
    tty->bofs = 0;
}