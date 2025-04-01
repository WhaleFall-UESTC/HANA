#include <common.h>
#include <klib.h>
#include <debug.h>
#include <riscv.h>
#include <syscall.h>
#include <trap/trap.h>
#include <trap/context.h>
#include <proc/proc.h>

#define ARG_CASE(n) case n: return tf->a##n

uint64 
argraw(int n)
{
    struct proc* p = myproc();
    struct trapframe *tf = p->trapframe;
    switch (n) {
        ARG_CASE(0);
        ARG_CASE(1);
        ARG_CASE(2);
        ARG_CASE(3);
        ARG_CASE(4);
        ARG_CASE(5);
    }
    panic("n: %d", n);
    return -1;
}


typedef uint64 (*syscall_handler)(void);

syscall_handler syscalls[NR_SYSCALL] = {
    
};


void
syscall()
{
    struct proc* p = myproc();
    int code = p->trapframe->a7;
    if (code > 0 && code < NR_SYSCALL && syscalls[code]) {
        p->trapframe->a0 = syscalls[code]();
    }
    else {
        panic("syscall");
    }
}