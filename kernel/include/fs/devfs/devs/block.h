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

extern ssize_t block_read(struct disk *disk, char *buffer, size_t size, off_t *offset);
extern ssize_t block_write(struct disk *disk, const char *buffer, size_t size, off_t *offset);
extern off_t block_llseek(struct disk *disk, off_t offset, int whence);
extern void block_init(struct disk* disk, struct blkdev* blkdev, const char* name);

#endif // __BLOCK_H__