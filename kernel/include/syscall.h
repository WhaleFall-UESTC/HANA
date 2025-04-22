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

#define SYSCALL_KERNEL_DEFINE(x, name, ...)                                  \
    uint64 call_sys##name(__MAP(x, __SC_DECL, __VA_ARGS__))                  \
    {                                                                        \
        return __do_sys##name(__MAP(x, __SC_CAST, _PARAMS(x, __VA_ARGS__))); \
    }

#define _SYSCALL_DEFINEx(x, name, ...)                                       \
    static inline uint64 __do_sys##name(__MAP(x, __SC_DECL, __VA_ARGS__));   \
    uint64 sys##name(void)                                                   \
    {                                                                        \
        return __do_sys##name(__MAP(x, __SC_CAST, _PARAMS(x, __VA_ARGS__))); \
    }                                                                        \
    SYSCALL_KERNEL_DEFINE(x, name, __VA_ARGS__)                              \
    static inline uint64 __do_sys##name(__MAP(x, __SC_DECL, __VA_ARGS__))

void syscall();

#ifdef ARCH_RISCV
#include <trap/syscall.h>
#endif
#endif // __SYSCALL_H__