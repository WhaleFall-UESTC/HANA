#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <common.h>

/* oscmop requires */

// FIle System
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

#define NR_SYSCALL 30

typedef uint64 (*syscall_func_t)(void);

#define _PARAM1(x, type, arg, ...) type, __sys_get_register(x)
#define _PARAM2(x, type, arg, ...) type, __sys_get_register(x), _PARAM1(metamacro_inc(x), __VA_ARGS__)
#define _PARAM3(x, type, arg, ...) type, __sys_get_register(x), _PARAM2(metamacro_inc(x), __VA_ARGS__)
#define _PARAM4(x, type, arg, ...) type, __sys_get_register(x), _PARAM3(metamacro_inc(x), __VA_ARGS__)
#define _PARAM5(x, type, arg, ...) type, __sys_get_register(x), _PARAM4(metamacro_inc(x), __VA_ARGS__)
#define _PARAM6(x, type, arg, ...) type, __sys_get_register(x), _PARAM5(metamacro_inc(x), __VA_ARGS__)

#define _PARAMS(n, ...) _PARAM##n(0, __VA_ARGS__)

#define __MAP0(m, ...)
#define __MAP1(m, t, a, ...) m(t, a)
#define __MAP2(m, t, a, ...) m(t, a), __MAP1(m, __VA_ARGS__)
#define __MAP3(m, t, a, ...) m(t, a), __MAP2(m, __VA_ARGS__)
#define __MAP4(m, t, a, ...) m(t, a), __MAP3(m, __VA_ARGS__)
#define __MAP5(m, t, a, ...) m(t, a), __MAP4(m, __VA_ARGS__)
#define __MAP6(m, t, a, ...) m(t, a), __MAP5(m, __VA_ARGS__)
#define __MAP(n, ...) __MAP##n(__VA_ARGS__)

#define __SC_DECL(t, a) t a
#define __TYPE_AS(t, v) __same_type((__force t)0, v)
#define __TYPE_IS_L(t) (__TYPE_AS(t, 0L))
#define __TYPE_IS_UL(t) (__TYPE_AS(t, 0UL))
#define __TYPE_IS_LL(t) (__TYPE_AS(t, 0LL) || __TYPE_AS(t, 0ULL))
#define __SC_LONG(t, a) __typeof(__builtin_choose_expr(__TYPE_IS_LL(t), 0LL, 0L)) a
#define __SC_CAST(t, a) (__force t) a
#define __SC_RESERVE(t, a) a

#define SYSCALL_DEFINE0(name, ret_type) _SYSCALL_DEFINEx(0, _##name, ret_type)
#define SYSCALL_DEFINE1(name, ret_type, ...) _SYSCALL_DEFINEx(1, _##name, ret_type, __VA_ARGS__)
#define SYSCALL_DEFINE2(name, ret_type, ...) _SYSCALL_DEFINEx(2, _##name, ret_type, __VA_ARGS__)
#define SYSCALL_DEFINE3(name, ret_type, ...) _SYSCALL_DEFINEx(3, _##name, ret_type, __VA_ARGS__)
#define SYSCALL_DEFINE4(name, ret_type, ...) _SYSCALL_DEFINEx(4, _##name, ret_type, __VA_ARGS__)
#define SYSCALL_DEFINE5(name, ret_type, ...) _SYSCALL_DEFINEx(5, _##name, ret_type, __VA_ARGS__)
#define SYSCALL_DEFINE6(name, ret_type, ...) _SYSCALL_DEFINEx(6, _##name, ret_type, __VA_ARGS__)

#define SYSCALL_KERNEL_DEFINE(x, name, ret_type, ...)               \
    ret_type call_sys##name(__MAP(x, __SC_DECL, __VA_ARGS__))       \
    {                                                               \
        return __do_sys##name(__MAP(x, __SC_RESERVE, __VA_ARGS__)); \
    }

#define _SYSCALL_DEFINEx(x, name, ret_type, ...)                                     \
    static inline ret_type __do_sys##name(__MAP(x, __SC_DECL, __VA_ARGS__));         \
    uint64 sys##name(void)                                                           \
    {                                                                                \
        return (uint64)__do_sys##name(__MAP(x, __SC_CAST, _PARAMS(x, __VA_ARGS__))); \
    }                                                                                \
    SYSCALL_KERNEL_DEFINE(x, name, ret_type, __VA_ARGS__)                                      \
    static inline ret_type __do_sys##name(__MAP(x, __SC_DECL, __VA_ARGS__))

void syscall();



/* mmap flags */

#define PROT_NONE      0x0
#define PROT_READ      0x1
#define PROT_WRITE     0x2
#define PROT_EXEC      0x4

#define MAP_SHARED     0x01     // 共享映射，对映射内容的修改会写回文件
#define MAP_PRIVATE    0x02     // 私有映射，写操作触发 Copy-on-Write（CoW）
#define MAP_FIXED      0x10     // 强制使用指定地址 addr，覆盖已有映射
#define MAP_ANONYMOUS  0x20     // 匿名映射，不基于文件，内容初始化为 0
// #define MAP_POPULATE   0x200000    // 预加载页面（避免缺页异常）
// #define MAP_STACK      0x40000  // 指示这是一个栈映射（提示系统进行优化）
// #define MAP_HUGETLB    0x400000 // 使用大页（huge page）进行映射（提高性能）

#define MMAP_FAILED ((void*) -1)

/* clone flags */

#define CLONE_VM             0x00000100  // 共享地址空间 (线程)
#define CLONE_FS             0x00000200  // 共享文件系统信息
#define CLONE_FILES          0x00000400  // 共享文件描述符表
#define CLONE_SIGHAND        0x00000800  // 共享信号处理程序
#define CLONE_THREAD         0x00010000  // 同线程组 (POSIX 线程)
#define CLONE_SYSVSEM        0x00040000  // 共享 System V 信号量
#define CLONE_SETTLS         0x00080000  // 设置 TLS (必须)
#define CLONE_PARENT_SETTID  0x00100000  // 将子线程 TID 写入 ptid
#define CLONE_CHILD_CLEARTID 0x00200000  // 子线程退出时清除 ctid
#define CLONE_CHILD_SETTID   0x01000000  // 将子线程 TID 写入 ctid
#define CLONE_SIGCHLD        0x00000011  // 子进程退出发送 SIGCHLD

#include <trap/syscall.h>
#endif // __SYSCALL_H__