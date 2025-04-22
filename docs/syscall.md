# 系统调用

## 注册系统调用方法

在`syscall.h`中定义了`SYSCALL_DEFINEx`宏，要定义的 syscall 只需根据参数个数替换x，并在宏的参数中依次填入每个参数的类型和参数的名称即可。

如要定义`sysopen(const char * filename, int flags, umode_t mode)`，只需：

```c
SYSCALL_DEFINE3(open, const char *, filename, int, flags, umode_t, mode) {
    /* DO SOMETHING */
}
```

在 riscv 中，他展开相当于：

```c
static inline uint64 __do_sys_open(const char * filename, int flags, umode_t mode);
uint64 sys_open(void)
{
    return __do_sys_open((const char *) myproc()->trapframe->a0, (int) myproc()->trapframe->a1, (umode_t) myproc()->trapframe->a2);
}
static inline uint64 __do_sys_open(const char * filename, int flags, umode_t mode)
{
    /* DO SOMETHING */
}
```

然后只需在系统调用的句柄数组中注册`sys_open`即可，可能需要在头文件中声明`extern uint64 sys_open(void)`。

参数个数的上限是 6 个，并且这种实现需要在每个架构中实现`__sys_get_register(n)`宏，他的展开是第`n`个参数传参的寄存器。