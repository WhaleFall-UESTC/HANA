#ifndef __TTY_H__
#define __TTY_H__

#include <common.h>
#include <io/chr.h>
#include <fs/fs.h>

#define TTY_BUF_SIZE_DEFAULT 4096

struct tty {
    struct chrdev* chrdev;
    char name[MAX_FILENAME_LEN];
    char *buffer; // NULL for no buffer
    size_t bofs, bufsize;
};

extern ssize_t tty_read(struct tty* tty, char *buffer, size_t size, off_t *offset);
extern ssize_t tty_write(struct tty* tty, const char *buffer, size_t size, off_t *offset);
extern off_t tty_llseek(struct tty* tty, off_t offset, int whence);
extern void tty_init(struct tty* tty, struct chrdev* chrdev, const char* name, size_t bufsize);

#endif // __TTY_H__