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

#define EXT4_BLK_DEV "/dev/sda"

int     call_sys_mount(const char *special, const char *dir, const char *fstype, unsigned long flags, const void *data);
int     call_sys_umount2(const char *special, int flags);
char*   call_sys_getcwd(char *buf, size_t size);
int     call_sys_chdir(const char *path);
int     call_sys_mkdirat(int dirfd, const char *path, umode_t mode);
int     call_sys_openat(int dirfd, const char *filename, int flags, umode_t mode);
int     call_sys_close(int fd);
ssize_t call_sys_write(int fd, const void *buf, size_t count);
ssize_t call_sys_read(int fd, void *buf, size_t count);
int     call_sys_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags);
int     call_sys_unlinkat(int dirfd, const char *path, unsigned int flags);
fd_t    call_sys_dup(fd_t oldfd);
fd_t    call_sys_dup3(fd_t oldfd, int newfd, int flags);
int     call_sys_getdents64(int fd, struct dirent *buf, size_t len);
int     call_sys_fstat(int fd, struct stat *kst);
int     call_sys_pipe2(int* pipefd, int flags);

void test_fs()
{
    struct stat kst;
    struct dirent *d;
    char *cwd;
    int ret, fd, new_fd, dir_fd;
    ssize_t n;
    char buf[128];
    int pipefd[2];

    log("FS test start!");

    if((ret = call_sys_pipe2(pipefd, 0)) != 0)
    {
        error("get pipe failed: %d", ret);
        return;
    }
    PASS("pipe2 success");

    const char *pdata = "testpippppppppppppe";
    if ((n = call_sys_write(pipefd[1], pdata, strlen(pdata))) != strlen(pdata))
    {
        error("write failed: %ld", n);
        return;
    }
    PASS("write pipe success");
    if ((n = call_sys_read(pipefd[0], buf, strlen(pdata))) != strlen(pdata))
    {
        error("read failed: %ld", n);
        return;
    }
    if (memcmp(buf, pdata, strlen(pdata)) != 0)
    {
        error("data mismatch");
        return;
    }
    PASS("read pipe success");
    if ((ret = call_sys_close(pipefd[0])) != 0)
    {
        error("close pipe failed: %d", ret);
        return;
    }
    if ((ret = call_sys_close(pipefd[1])) != 0)
    {
        error("close pipe failed: %d", ret);
        return;
    }
    PASS("close pipe success");

    if ((fd = call_sys_openat(AT_FDCWD, "/dev", O_RDONLY, 0)) < 0)
    {
        error("create testfile failed: %d", fd);
        return;
    }
    PASS("open /dev");
    char *devdirbuf = kalloc(512);
    if ((n = call_sys_getdents64(fd, (struct dirent *)devdirbuf, 512)) <= 0)
    {
        error("getdents64 failed: %ld", n);
        kfree(devdirbuf);
        return;
    }
    for (int pos = 0; pos < n; pos += d->d_reclen)
    {
        d = (struct dirent *)(devdirbuf + pos);
        log("%s", d->d_name);
    }
    kfree(devdirbuf);
    call_sys_close(fd);
    PASS("getdents64 success");

    // 挂载文件系统
    if ((ret = call_sys_mount(EXT4_BLK_DEV, "/", "ext4", 0, NULL)) != 0)
    {
        error("mount failed: %d", ret);
        return;
    }
    PASS("mount / success");
    // 测试初始工作目录
    if ((cwd = call_sys_getcwd(buf, 128)) == NULL)
    {
        error("getcwd failed");
        goto umount;
    }
    if (strcmp(cwd, "/") != 0)
    {
        error("initial cwd mismatch: %s", cwd);
        goto umount;
    }
    PASS("initial cwd is /");
    // 创建测试目录
    if ((ret = call_sys_mkdirat(AT_FDCWD, "/testdir", 0755)) != 0)
    {
        error("mkdir /testdir failed: %d", ret);
        goto umount;
    }
    PASS("mkdir /testdir success");
    // 切换工作目录
    if ((ret = call_sys_chdir("/testdir")) != 0)
    {
        error("chdir to /testdir failed: %d", ret);
        goto cleanup_dir;
    }
    PASS("chdir to /testdir success");
    // 验证工作目录
    if ((cwd = call_sys_getcwd(buf, 128)) == NULL || strcmp(cwd, "/testdir") != 0)
    {
        error("cwd verify failed: %s", cwd ? cwd : "NULL");
        goto cleanup_chdir;
    }
    PASS("cwd after chdir is /testdir");
    // 创建测试文件
    if ((fd = call_sys_openat(AT_FDCWD, "testfile", O_CREAT | O_RDWR, 0644)) < 0)
    {
        error("create testfile failed: %d", fd);
        goto cleanup_chdir;
    }
    PASS("create testfile success");
    // 写入测试数据
    const char *data = "testdata";
    if ((n = call_sys_write(fd, data, strlen(data))) != strlen(data))
    {
        error("write failed: %ld", n);
        goto cleanup_file;
    }
    PASS("write testfile success");
    // 关闭文件
    if ((ret = call_sys_close(fd)) != 0)
    {
        error("close failed: %d", ret);
        goto cleanup_file;
    }
    PASS("close testfile success");
    // 重新打开验证内容
    if ((fd = call_sys_openat(AT_FDCWD, "testfile", O_RDONLY, 0)) < 0)
    {
        error("reopen testfile failed: %d", fd);
        goto cleanup_file;
    }
    if ((n = call_sys_read(fd, buf, sizeof(buf))) != strlen(data))
    {
        error("read failed: %ld", n);
        goto cleanup_file;
    }
    if (memcmp(buf, data, strlen(data)) != 0)
    {
        error("data mismatch");
        goto cleanup_file;
    }
    PASS("read testfile success");
    call_sys_close(fd);
    // 创建硬链接
    if ((ret = call_sys_linkat(AT_FDCWD, "testfile", AT_FDCWD, "linkfile", 0)) != 0)
    {
        error("link failed: %d", ret);
        goto cleanup_file;
    }
    PASS("link success");
    // 验证链接文件
    if ((fd = call_sys_openat(AT_FDCWD, "linkfile", O_RDONLY, 0)) < 0)
    {
        error("open linkfile failed: %d", fd);
        goto cleanup_link;
    }
    call_sys_close(fd);
    PASS("linkfile valid");
    // 测试dup/dup3
    fd = call_sys_openat(AT_FDCWD, "testfile", O_RDONLY, 0);
    if ((new_fd = call_sys_dup(fd)) < 0)
    {
        error("dup failed: %d", new_fd);
        goto cleanup_link;
    }
    call_sys_close(new_fd);
    if ((new_fd = call_sys_dup3(fd, 100, 0)) != 100)
    {
        error("dup3 failed: %d", new_fd);
        goto cleanup_link;
    }
    call_sys_close(new_fd);
    call_sys_close(fd);
    PASS("dup/dup3 success");
    // 测试目录遍历
    dir_fd = call_sys_openat(AT_FDCWD, ".", O_RDONLY | O_DIRECTORY, 0);
    char *dirbuf = kalloc(512);
    if ((n = call_sys_getdents64(dir_fd, (struct dirent *)dirbuf, 512)) <= 0)
    {
        error("getdents64 failed: %ld", n);
        kfree(dirbuf);
        goto cleanup_link;
    }
    int found = 0;
    for (int pos = 0; pos < n; pos += d->d_reclen)
    {
        d = (struct dirent *)(dirbuf + pos);
        if (strcmp(d->d_name, "testfile") == 0)
            found++;
    }
    if (!found)
    {
        error("directory entry not found");
        kfree(dirbuf);
        goto cleanup_link;
    }
    kfree(dirbuf);
    call_sys_close(dir_fd);
    PASS("getdents64 success");
    // 测试fstat
    fd = call_sys_openat(AT_FDCWD, "testfile", O_RDONLY, 0);
    if (call_sys_fstat(fd, &kst) != 0 || kst.st_size != strlen(data))
    {
        error("fstat failed");
        call_sys_close(fd);
        goto cleanup_link;
    }
    call_sys_close(fd);
    PASS("fstat success");
// 清理流程
cleanup_link:
    call_sys_unlinkat(AT_FDCWD, "linkfile", 0);
cleanup_file:
    call_sys_unlinkat(AT_FDCWD, "testfile", 0);
cleanup_chdir:
    call_sys_chdir("/");
cleanup_dir:
    call_sys_unlinkat(AT_FDCWD, "/testdir", AT_REMOVEDIR);
umount:
    call_sys_umount2("/", 0);
}