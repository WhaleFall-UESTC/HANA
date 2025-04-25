# 系统调用

## 注册系统调用方法

在`syscall.h`中定义了`SYSCALL_DEFINEx`宏，要定义的 syscall 只需根据参数个数替换x，并在宏的参数中先填入返回值，再依次填入每个参数的类型和参数的名称即可。

如要定义`fd_t sys_openat(fd_t dirfd, const char* path, int flags, umode_t mode); `，只需：

```c
SYSCALL_DEFINE4(openat, fd_t, fd_t, dirfd, const char*, path, int, flags, umode_t, mode) {
    /* DO SOMETHING */
}
```

在 riscv 中，他展开相当于：

```c
static inline fd_t __do_sys_openat(fd_t dirfd, const char *path, int flags, umode_t mode);
uint64 sys_openat(void)
{
    return (uint64)__do_sys_openat((fd_t)myproc()->trapframe->a0, (const char *)myproc()->trapframe->a1, (int)myproc()->trapframe->a2, (umode_t)myproc()->trapframe->a3);
}
fd_t call_sys_openat(fd_t dirfd, const char *path, int flags, umode_t mode) // ONLY WHEN DEBUG ENABLED
{
    return __do_sys_openat(dirfd, path, flags, mode);
}
static inline fd_t __do_sys_openat(fd_t dirfd, const char *path, int flags, umode_t mode) {
    /* DO SOMETHING */
}
```

然后只需在系统调用的句柄数组中注册`sys_open`即可，可能需要在头文件中声明`extern fd_t sys_open(void)`。

开启调试时，会生成`call_`开头的函数，其原型可以在源文件中声明之后直接调用。

参数个数的上限是 6 个，并且这种实现需要在每个架构中实现`__sys_get_register(n)`宏，他的展开是第`n`个参数传参的寄存器。