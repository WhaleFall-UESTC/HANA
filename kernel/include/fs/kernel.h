#ifndef __KERNEL_FS_H__
#define __KERNEL_FS_H__

#include <common.h>
#include <fs/fs.h>
#include <fs/file.h>

struct file* kernel_open(const char* path);
ssize_t kernel_read(struct file* file, void* buf, size_t size);
ssize_t kernel_write(struct file* file, const void* buf, size_t size);
int kernel_close(struct file* file);

#endif // __KERNEL_FS_H__