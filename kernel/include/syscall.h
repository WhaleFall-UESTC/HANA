#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <common.h>

enum
{
    SYS_BLANK,

    // File Operations
    SYS_open,
    SYS_read,
    SYS_write,
    SYS_close,
    SYS_lseek,
    SYS_stat,
    SYS_fstat,
    SYS_unlink,
    SYS_mkdir,
    SYS_rmdir,
    SYS_link,
    SYS_symlink,
    SYS_access,

    // Process Management
    SYS_fork,
    SYS_execve,
    SYS_exit,
    SYS_wait4,
    SYS_getpid,
    SYS_getppid,
    SYS_kill,

    // IPC
    SYS_pipe,
    SYS_shmget,
    SYS_shmat,
    SYS_msgget,
    SYS_msgsnd,

    // Memory Management
    SYS_brk,
    SYS_mmap,
    SYS_munmap,
    SYS_mprotect,

    // Time
    SYS_time,
    SYS_gettimeofday,
    SYS_nanosleep,

    // Signals
    SYS_sigaction,
    SYS_sigprocmask,

    NR_SYSCALL
};

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

#define SYSCALL_DEFINE0(name) _SYSCALL_DEFINEx(0, _##name)
#define SYSCALL_DEFINE1(name, ...) _SYSCALL_DEFINEx(1, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE2(name, ...) _SYSCALL_DEFINEx(2, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE3(name, ...) _SYSCALL_DEFINEx(3, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE4(name, ...) _SYSCALL_DEFINEx(4, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE5(name, ...) _SYSCALL_DEFINEx(5, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE6(name, ...) _SYSCALL_DEFINEx(6, _##name, __VA_ARGS__)

#define _SYSCALL_DEFINEx(x, name, ...)                                       \
    static inline uint64 __do_sys##name(__MAP(x, __SC_DECL, __VA_ARGS__));   \
    uint64 sys##name(void)                                                   \
    {                                                                        \
        return __do_sys##name(__MAP(x, __SC_CAST, _PARAMS(x, __VA_ARGS__))); \
    }                                                                        \
    static inline uint64 __do_sys##name(__MAP(x, __SC_DECL, __VA_ARGS__))

void syscall();

#ifdef ARCH_RISCV
#include <trap/syscall.h>
#endif
#endif // __SYSCALL_H__