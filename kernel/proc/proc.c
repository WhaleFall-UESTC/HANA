#ifndef __PROC_C__
#define __PROC_C__

#include <common.h>
#include <klib.h>
#include <debug.h>

#ifdef ARCH_RISCV
#include <riscv.h>
#endif

#include <context.h>
#include <proc/proc.h>
#include <trap.h>
#include <mm/mm.h>
#include <mm/memlayout.h>

extern struct proc* proc_list;

volatile int next_pid = 1;

// dead loop
char init_code[] = {0x67, 0x00, 0x00, 0x00};

int
alloc_pid()
{
    return next_pid++;
}


// alloc a new proc and initialize it
// which can be sched after init
struct proc*
alloc_proc()
{
    KALLOC(struct proc, p);
    
    p->pid = alloc_pid();
    p->stack = KSTACK(p->pid);

    void* stack = kalloc(KSTACK_SIZE);
    Assert(stack, "out of memory");
    mappages(kernel_pagetable, p->stack, (uint64)stack, KSTACK_SIZE, PTE_R | PTE_W);

    p->state = INIT;

    p->trapframe = (struct trapframe*) kalloc(sizeof(struct trapframe));
    Assert(p->trapframe, "out of memory");

    memset(&p->context, 0, sizeof(struct context));
    // leave space for pt_regs
    p->context.sp = p->stack + KSTACK_SIZE;
    p->context.ra = (uint64) dive_to_user; 
    
    p->next = NULL;

    return p;
}


void
proc_init()
{
    struct proc* p = alloc_proc();

    // user vm space init
    p->pagetable = uvminit((uint64)p->trapframe, init_code, sizeof(init_code));
    p->sz = 2*PGSIZE;

    p->trapframe->epc = 0;
    p->trapframe->sp = 3*PGSIZE;

    strcpy(p->name, "init");
    
    p->state = RUNNABLE;

    proc_list = p;
}


#endif // __PROC_C__

