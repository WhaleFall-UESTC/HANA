#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <common.h>
#include <io/blk.h>
#include <fs/fs.h>

struct disk {
    struct blkdev* blkdev;
    char name[MAX_FILENAME_LEN];
    char *buffer; // io buffer in sector size, used for last sector
    off_t ofs;
};

/**
 * Read data from a block device
 * @param disk: The disk structure representing the block device
 * @param buffer: The buffer to store the read data
 * @param size: The number of bytes to read
 * @param offset: Pointer to the current offset (will be updated after reading)
 * @return Number of bytes read, or -1 on error
 */
ssize_t block_read(struct disk *disk, char *buffer, size_t size, off_t *offset);

/**
 * Write data to a block device
 * @param disk: The disk structure representing the block device
 * @param buffer: The buffer containing data to write
 * @param size: The number of bytes to write
 * @param offset: Pointer to the current offset (will be updated after writing)
 * @return Number of bytes written, or -1 on error
 */
ssize_t block_write(struct disk *disk, const char *buffer, size_t size, off_t *offset);

/**
 * Change the current offset for a block device
 * @param disk: The disk structure representing the block device
 * @param offset: The new offset value
 * @param whence: Reference position (SEEK_SET, SEEK_CUR, SEEK_END)
 * @return The new absolute offset, or -1 on error
 */
off_t block_llseek(struct disk *disk, off_t offset, int whence);

/**
 * Initialize a disk structure
 * @param disk: The disk structure to initialize
 * @param blkdev: The underlying block device
 * @param name: Name for the disk device
 */
void block_init(struct disk* disk, struct blkdev* blkdev, const char* name);

#endif // __BLOCK_H__