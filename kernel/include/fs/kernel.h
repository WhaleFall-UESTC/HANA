#ifndef __KERNEL_FS_H__
#define __KERNEL_FS_H__

/**
 * This file provides kernel-level file operations.
 */

#include <common.h>
#include <fs/fs.h>
#include <fs/file.h>

/**
 * Open a file in the kernel.
 * @param path: The path to the file to open.
 * @return: A pointer to the opened file structure, or NULL on error.
 */
struct file* kernel_open(const char* path);

/**
 * Open a file relative to a directory file descriptor.
 * @param dfd: The directory file descriptor.
 * @param path: The path to the file to open, relative to dfd.
 * @return: A pointer to the opened file structure, or NULL on error.
 */
struct file* kernel_openat(fd_t dfd, const char* path);

/**
 * Read data from a file in the kernel.
 * @param file: The file structure to read from.
 * @param buf: The buffer to read data into.
 * @param size: The number of bytes to read.
 * @return: The number of bytes read, or -1 on error.
 */
ssize_t kernel_read(struct file* file, void* buf, size_t size);

/**
 * Write data to a file in the kernel.
 * @param file: The file structure to write to.
 * @param buf: The buffer containing data to write.
 * @param size: The number of bytes to write.
 * @return: The number of bytes written, or -1 on error.
 */
ssize_t kernel_write(struct file* file, const void* buf, size_t size);

/**
 * Close a file in the kernel.
 * @param file: The file structure to close.
 * @return: 0 on success, -1 on error.
 */
int kernel_close(struct file* file);

/**
 * Seek to a position in a file.
 * @param file: The file structure to seek in.
 * @param offset: The offset to seek to.
 * @param whence: The reference point for the offset (e.g., SEEK_SET, SEEK_CUR, SEEK_END).
 * @return: The new file position on success, or -1 on error.
 */
off_t kernel_lseek(struct file* file, off_t offset, int whence);

/**
 * Mount a file system in the kernel.
 * special and dir must be absolute here
 * @param special: The special device file to mount (e.g., /dev/sda1).
 * @param dir: The directory where the file system should be mounted (e.g., /mnt).
 * @param fstype: The type of the file system (e.g., "ext4").
 * @param flags: Mount flags (e.g., MS_RDONLY).
 * @param data: Optional data for the file system (can be NULL).
 * @return: 0 on success, -1 on error.
 */
int kernel_mount(const char * special, const char * dir, const char * fstype, unsigned long flags, const void * data);

#endif // __KERNEL_FS_H__