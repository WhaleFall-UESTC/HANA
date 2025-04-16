#ifndef __FS_H__
#define __FS_H__

#include <common.h>
#include <io/blk.h>

#include <lwext4/ext4.h>
#include <lwext4/ext4_blockdev.h>

int ext4_init(void);
int dir_open(struct ext4_dir *dir, const char *path);
int fclose(struct ext4_file *file);
int fname(struct ext4_file *f, char *path);
int fopen(struct ext4_file *file, const char *path, uint32_t flags);
int fread(struct ext4_file *file, uint64 buf, uint off, uint size, int *rcnt);
int fseek(struct ext4_file *file, uint off, uint origin);
int fkstatat(char *path, struct kstat *kst, int dirfd);
int funlink(const char *path);
int fwrite(struct ext4_file *file, uint64 buf, uint off, uint size, int *wcnt);
int fwritev(struct ext4_file *f, struct iovec iov[], int iovcnt, uint off);
int freadv(struct ext4_file *f, struct iovec iov[], int iovcnt, uint off);
uint32 get_inode_type(struct ext4_file *file);
int mkdir(const char *path);
int dir_close(struct ext4_dir *dir);

#endif // __FS_H__