#include <common.h>
#include <klib.h>
#include <debug.h>
#include <arch.h>
#include <syscall.h>
#include <irq/interrupt.h>
#include <trap/trap.h>
#include <trap/context.h>
#include <proc/proc.h>

#define EXTERN_SYS(sys_name)    extern uint64 sys_##sys_name(void);
#define REGISTER_SYS(sys_name)  [SYS_##sys_name] = (syscall_func_t)sys_##sys_name,
#define MAP(s, f) s(f)

MAP(SYSCALLS, EXTERN_SYS)

syscall_func_t syscalls[] = {
    MAP(SYSCALLS, REGISTER_SYS)
};

void
syscall()
{
    struct proc* p = myproc();
    int code = p->trapframe->a7;

    if (code > 0 && syscalls[code]) {
        p->trapframe->a0 = syscalls[code]();
    }
    else {
        panic("unknown syscall %d", code);
    }

    p->trapframe->era += 4;
}
