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

uint64 call_sys_mkdir(const char* path, umode_t mode);
uint64 call_sys_open(const char* path, unsigned int flags);
uint64 call_sys_getdents64(int fd, struct dirent * buf, size_t len);

void test_fs(void) {
    int ret;
    fd_t fd;

    char buf[1024];

    log("test fs");

    // blkdev_get_by_name("virtio-blk1")->ops->status(blkdev_get_by_name("virtio-blk1"));

    ret = mount_root(EXT4_BLK_DEV, &ext4_fs);
    if (ret != 0) {
        error("mount root failed");
        return;
    }
    else PASS("mount root success");

    // ret = call_sys_mkdir("/test", 0777);
    // if (ret != 0) {
    //     error("mkdir failed");
    //     return;
    // }
    // else PASS("mkdir success");

    // ret = call_sys_mkdir("/test/aaa", 0777);
    // if (ret != 0)
    // {
    //     error("mkdir failed");
    //     return;
    // }
    // else
    //     PASS("mkdir success");

    fd = call_sys_open("/test", O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        error("open directory failed");
        return;
    }
    else {
        PASS("open directory success");
        log("open directory fd: %d", fd);
    }

    ret = call_sys_getdents64(fd, (struct dirent*)buf, 1000);
    if (ret < 0) {
        error("get dent failed");
        return;
    }
    else PASS("get dent success");

    struct dirent* dent = (struct dirent*)buf;
    log("get dent: %s", dent->d_name);
    log("get dent: %lu", dent->d_ino);
    log("get dent: %d", dent->d_reclen);
    log("get dent: %d", dent->d_type);
    log("get dent: %ld", dent->d_off);
}