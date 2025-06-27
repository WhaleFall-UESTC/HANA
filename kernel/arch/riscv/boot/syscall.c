#include <common.h>
#include <klib.h>
#include <debug.h>
#include <arch.h>
#include <syscall.h>
#include <irq/interrupt.h>
#include <trap/trap.h>
#include <trap/context.h>
#include <proc/proc.h>

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
}
