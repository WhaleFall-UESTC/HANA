#include <common.h>
#include <klib.h>
#include <debug.h>
#include <riscv.h>
#include <syscall.h>
#include <irq/interrupt.h>
#include <trap/trap.h>
#include <trap/context.h>
#include <proc/proc.h>

// extern uint64 sys_fork();

static uint64 (*syscalls[NR_SYSCALL])(void) = {
    // [SYS_fork] sys_fork,
};

// syscall_func_t syscalls[NR_SYSCALL];

void
syscall()
{
    struct proc* p = myproc();
    int code = p->trapframe->a7;

    if (code > 0 && code < NR_SYSCALL) {
        if (syscalls[code] == NULL)
            panic("unregistered syscall %d", code);
        p->trapframe->a0 = syscalls[code]();
    }
    else {
        panic("unknown syscall %d", code);
    }
}