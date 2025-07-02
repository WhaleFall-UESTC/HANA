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

/**
 * Read data from a TTY device
 * @param tty: The TTY structure representing the character device
 * @param buffer: The buffer to store the read data
 * @param size: The number of bytes to read
 * @param offset: Pointer to the current offset (will be updated after reading)
 * @return Number of bytes read
 */
ssize_t tty_read(struct tty* tty, char *buffer, size_t size, off_t *offset);

/**
 * Write data to a TTY device
 * @param tty: The TTY structure representing the character device
 * @param buffer: The buffer containing data to write
 * @param size: The number of bytes to write
 * @param offset: Pointer to the current offset (will be updated after writing)
 * @return Number of bytes written
 */
ssize_t tty_write(struct tty* tty, const char *buffer, size_t size, off_t *offset);

/**
 * Change the current offset for a TTY device (not supported)
 * @param tty: The TTY structure
 * @param offset: The offset value
 * @param whence: Reference position
 * @return Always returns -1 (not supported)
 */
off_t tty_llseek(struct tty* tty, off_t offset, int whence);

/**
 * Initialize a TTY structure
 * @param tty: The TTY structure to initialize
 * @param chrdev: The underlying character device
 * @param name: Name for the TTY device
 * @param bufsize: Size of the output buffer (0 for unbuffered)
 */
void tty_init(struct tty* tty, struct chrdev* chrdev, const char* name, size_t bufsize);


#endif // __TTY_H__