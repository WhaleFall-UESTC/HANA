#ifndef __U_SYSCALL_H__
#define __U_SYSCALL_H__

#define SYS_getcwd 17
#define SYS_pipe2 59
#define SYS_dup 23
#define SYS_dup3 24
#define SYS_chdir 49
#define SYS_openat 56
#define SYS_close 57
#define SYS_getdents64 61
#define SYS_read 63
#define SYS_write 64
#define SYS_linkat 37
#define SYS_unlinkat 35
#define SYS_mkdirat 34
#define SYS_umount2 39
#define SYS_mount 40
#define SYS_fstat 80

// Process Management
#define SYS_clone 220
#define SYS_execve 221
#define SYS_wait4 260
#define SYS_exit 93
#define SYS_getppid 173
#define SYS_getpid 172

// Memory Management
#define SYS_brk 214
#define SYS_munmap 215
#define SYS_mmap 222

// Others
#define SYS_times 153
#define SYS_uname 160
#define SYS_sched_yield 124
#define SYS_gettimeofday 169
#define SYS_nanosleep 101

typedef unsigned long uint64;
typedef unsigned long size_t;
typedef unsigned long ssize_t;
typedef unsigned long clock_t;
typedef unsigned mode_t;
typedef long off_t;
typedef int pid_t;

#if defined(__riscv)
static inline uint64 internal_syscall(long n, uint64 _a0, uint64 _a1, uint64 _a2, uint64
		_a3, uint64 _a4, uint64 _a5) {
	register uint64 a0 asm("a0") = _a0;
	register uint64 a1 asm("a1") = _a1;
	register uint64 a2 asm("a2") = _a2;
	register uint64 a3 asm("a3") = _a3;
	register uint64 a4 asm("a4") = _a4;
	register uint64 a5 asm("a5") = _a5;
	register long syscall_id asm("a7") = n;
	asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"
			(a5), "r"(syscall_id));
	return a0;
}
#else if defined(__loongarch)
static inline uint64 internal_syscall(long n, uint64 a0, uint64 a1, uint64 a2,
                                    uint64 a3, uint64 a4, uint64 a5) {
    register uint64 x0 asm("$a0") = a0; // 参数寄存器 $a0 - $a5
    register uint64 x1 asm("$a1") = a1;
    register uint64 x2 asm("$a2") = a2;
    register uint64 x3 asm("$a3") = a3;
    register uint64 x4 asm("$a4") = a4;
    register uint64 x5 asm("$a5") = a5;
    register long syscall_id asm("$a7") = n; // syscall number in $a7

    asm volatile (
        "syscall\n" // LoongArch 的 syscall 指令
        : "+r"(x0)
        : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(syscall_id)
        : "$t0", "$t1", "$t2", "$t3", "memory"
    );
    return x0;
}
#endif

// 文件系统相关
static inline char *getcwd(char *buf, size_t size) {
    return (char *) internal_syscall(SYS_getcwd, (uint64) buf, (uint64) size, 0, 0, 0, 0);
}

static inline int pipe2(int fd[2], int flags) {
    return (int) internal_syscall(SYS_pipe2, (uint64) fd, (uint64) flags, 0, 0, 0, 0);
}

static inline int dup(int oldfd) {
    return (int) internal_syscall(SYS_dup, (uint64) oldfd, 0, 0, 0, 0, 0);
}

static inline int dup3(int oldfd, int newfd, int flags) {
    return (int) internal_syscall(SYS_dup3, (uint64) oldfd, (uint64) newfd, (uint64) flags, 0, 0, 0);
}

static inline int chdir(const char *path) {
    return (int) internal_syscall(SYS_chdir, (uint64) path, 0, 0, 0, 0, 0);
}

static inline int openat(int dirfd, const char *pathname, int flags, mode_t mode) {
    return (int) internal_syscall(SYS_openat, (uint64) dirfd, (uint64) pathname, (uint64) flags, (uint64) mode, 0, 0);
}

static inline int close(int fd) {
    return (int) internal_syscall(SYS_close, (uint64) fd, 0, 0, 0, 0, 0);
}

static inline ssize_t getdents64(int fd, struct dirent *dirp, size_t count) {
    return (ssize_t) internal_syscall(SYS_getdents64, (uint64) fd, (uint64) dirp, (uint64) count, 0, 0, 0);
}

static inline ssize_t read(int fd, void *buf, size_t count) {
    return (ssize_t) internal_syscall(SYS_read, (uint64) fd, (uint64) buf, (uint64) count, 0, 0, 0);
}

#define STDOUT_FILENO 1
#define STDERR_FILENO 2

static inline ssize_t write(int fd, const void *buf, size_t count) {
    return (ssize_t) internal_syscall(SYS_write, (uint64) fd, (uint64) buf, (uint64) count, 0, 0, 0);
}

static inline int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) {
    return (int) internal_syscall(SYS_linkat, (uint64) olddirfd, (uint64) oldpath,
                                  (uint64) newdirfd, (uint64) newpath, (uint64) flags, 0);
}

static inline int unlinkat(int dirfd, const char *pathname, int flags) {
    return (int) internal_syscall(SYS_unlinkat, (uint64) dirfd, (uint64) pathname, (uint64) flags, 0, 0, 0);
}

static inline int mkdirat(int dirfd, const char *pathname, mode_t mode) {
    return (int) internal_syscall(SYS_mkdirat, (uint64) dirfd, (uint64) pathname, (uint64) mode, 0, 0, 0);
}

static inline int umount2(const char *target, int flags) {
    return (int) internal_syscall(SYS_umount2, (uint64) target, (uint64) flags, 0, 0, 0, 0);
}

static inline int mount(const char *source, const char *target,
                        const char *filesystemtype, unsigned long mountflags,
                        const void *data) {
    return (int) internal_syscall(SYS_mount, (uint64) source, (uint64) target,
                                  (uint64) filesystemtype, (uint64) mountflags, (uint64) data, 0);
}

static inline int fstat(int fd, struct stat *statbuf) {
    return (int) internal_syscall(SYS_fstat, (uint64) fd, (uint64) statbuf, 0, 0, 0, 0);
}

// 进程管理
static inline pid_t clone(unsigned long flags, void *stack, pid_t *parent_tid, void *tls, pid_t *child_tid) {
    return (pid_t) internal_syscall(SYS_clone, flags, (uint64) stack, (uint64) parent_tid, (uint64) tls, (uint64) child_tid, 0);
}

static inline int execve(const char *filename, char *const argv[], char *const envp[]) {
    return (int) internal_syscall(SYS_execve, (uint64) filename, (uint64) argv, (uint64) envp, 0, 0, 0);
}

static inline pid_t wait4(pid_t pid, int *status, int options, struct rusage *rusage) {
    return (pid_t) internal_syscall(SYS_wait4, (uint64) pid, (uint64) status, (uint64) options, (uint64) rusage, 0, 0);
}

static inline void _exit(int status) {
    internal_syscall(SYS_exit, (uint64) status, 0, 0, 0, 0, 0);
}

static inline pid_t getppid(void) {
    return (pid_t) internal_syscall(SYS_getppid, 0, 0, 0, 0, 0, 0);
}

static inline pid_t getpid(void) {
    return (pid_t) internal_syscall(SYS_getpid, 0, 0, 0, 0, 0, 0);
}

// 内存管理
static inline void *brk(void *addr) {
    return (void *) internal_syscall(SYS_brk, (uint64) addr, 0, 0, 0, 0, 0);
}

static inline int munmap(void *start, size_t length) {
    return (int) internal_syscall(SYS_munmap, (uint64) start, (uint64) length, 0, 0, 0, 0);
}

static inline void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return (void *) internal_syscall(SYS_mmap, (uint64) addr, (uint64) length,
                                     (uint64) prot, (uint64) flags, (uint64) fd, (uint64) offset);
}

// 时间与调度
static inline clock_t times(struct tms *buf) {
    return (clock_t) internal_syscall(SYS_times, (uint64) buf, 0, 0, 0, 0, 0);
}

static inline int uname(struct utsname *buf) {
    return (int) internal_syscall(SYS_uname, (uint64) buf, 0, 0, 0, 0, 0);
}

static inline int sched_yield(void) {
    return (int) internal_syscall(SYS_sched_yield, 0, 0, 0, 0, 0, 0);
}

static inline int gettimeofday(struct timeval *tv, void *tz) {
    return (int) internal_syscall(SYS_gettimeofday, (uint64) tv, (uint64) tz, 0, 0, 0, 0);
}

static inline int nanosleep(const struct timespec *req, struct timespec *rem) {
    return (int) internal_syscall(SYS_nanosleep, (uint64) req, (uint64) rem, 0, 0, 0, 0);
}


#endif // __U_SYSCALL_H__
