#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/stat.h>
#include <fs/io.h>
#include <fs/dirent.h>
#include <fs/ext4/ext4.h>
#include <fs/ext4/ext4_blk.h>

#include <io/blk.h>
#include <proc/proc.h>
#include <klib.h>
#include <mm/mm.h>
#include <debug.h>
#include <syscall.h>
#include <common.h>
#include <locking/spinlock.h>

#define EXT4_BLK_DEV "virtio-blk1"

uint64 call_sys_mkdir(const char *path, umode_t mode);
uint64 call_sys_open(const char *path, unsigned int flags);
uint64 call_sys_getdents64(int fd, struct dirent *buf, size_t len);
uint64 call_sys_close(int fd);
uint64 call_sys_read(int fd, char *buf, size_t count);
uint64 call_sys_write(int fd, const char *buf, size_t count);

void test_fs(void)
{
    int ret;
    fd_t fd;

    char buf[1024];
    memset(buf, 0, sizeof(buf));

    log("test fs");

    // blkdev_get_by_name("virtio-blk1")->ops->status(blkdev_get_by_name("virtio-blk1"));

    ret = mount_root(EXT4_BLK_DEV, &ext4_fs);
    if (ret != 0)
    {
        error("mount root failed");
        return;
    }
    else
        PASS("mount root success");

    // ret = call_sys_mkdir("/dir1", 0777);
    // if (ret != 0)
    // {
    //     error("mkdir failed");
    //     return;
    // }
    // else
    //     PASS("mkdir success");

    ret = call_sys_mkdir("/dir2/aaa", 0777);
    if (ret != 0)
    {
        error("mkdir failed");
        return;
    }
    else
        PASS("mkdir success");

    fd = call_sys_open("/test/hhh", O_RDWR | O_CREAT);
    if (fd < 0)
    {
        error("open file failed");
        return;
    }
    else
    {
        PASS("open file success");
        log("open file fd: %d", fd);
    }

    ret = call_sys_write(fd, "hello world", 11);
    if (ret < 0)
    {
        error("write file failed");
        return;
    }
    else
        PASS("write file success");

    ret = call_sys_read(fd, buf, 1024);
    if (ret < 0)
    {
        error("read file failed");
        return;
    }
    else
        PASS("read file success");

    log("read file: %s", buf);

    ret = call_sys_close(fd);
    if (ret < 0)
    {
        error("close file failed");
        return;
    }
    else
        PASS("close file success");

    fd = call_sys_open("/test", O_RDONLY | O_DIRECTORY);
    if (fd < 0)
    {
        error("open directory failed");
        return;
    }
    else
    {
        PASS("open directory success");
        log("open directory fd: %d", fd);
    }

    ret = call_sys_getdents64(fd, (struct dirent *)buf, 1000);
    if (ret < 0)
    {
        error("get dent failed");
        return;
    }
    else
        PASS("get dent success");

    for (struct dirent *dent = (struct dirent *)buf; ;
         dent = (struct dirent *)((uint64)dent + dent->d_reclen))
    {
        log("get dent: name %s", dent->d_name);
        log("get dent: ino %lu", dent->d_ino);
        log("get dent: reclen %d", dent->d_reclen);
        log("get dent: dtype %d", dent->d_type);
        log("get dent: doff %ld\n", dent->d_off);
        if (dent->d_off == -1) break;
    }

    ret = call_sys_close(fd);
    if (ret < 0)
    {
        error("close directory failed");
        return;
    }
    else
        PASS("close directory success");

    // ret = call_sys_mkdir("/test/aaa", 0777);
    // if (ret != 0) {
    //     error("mkdir failed");
    //     return;
    // }
    // else PASS("mkdir success");
}