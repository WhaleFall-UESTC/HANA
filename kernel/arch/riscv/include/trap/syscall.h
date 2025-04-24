#ifndef __RISCV_SYSCALL_H__
#define __RISCV_SYSCALL_H__

#include <common.h>

#define __sys_get_register(n) \
    myproc()->trapframe->a##n

#define argraw(n) \
    p->trapframe->a##n

#endif // __RISCV_SYSCALL_H__