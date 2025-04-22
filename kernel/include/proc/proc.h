#ifndef __PROC_H__
#define __PROC_H__

#ifdef ARCH_RISCV
#include <trap/context.h>
#include <trap/trap.h>
#endif

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__
#include <irq/interrupt.h>
#endif

struct proc {
    int pid;
    volatile int state;

    uint64 sz;

    pagetable_t pagetable;
    uint64 stack;
    struct trapframe* trapframe;
    struct context context;

    struct files_struct* fdt;
    char* cwd; // current working directory

    int killed;
    void* chan;     // proc sleep on which channel
    int status;     // exit() return status

    struct proc* parent;

    struct proc* next;
    
    char name[16];
};

enum proc_state{ INIT, SLEEPING, RUNNABLE, RUNNING, ZOMBIE, NR_PROC_STATE };

struct cpu {
    struct context context;
    struct proc* proc;
    int noff;       // Depth of push_off() nesting.
    int intena;     // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

#define CPUID(c) (int)(((uint64)((c) - cpus))/sizeof(struct cpu))

static inline struct cpu*
mycpu() 
{
    return &cpus[r_tp()];
}

static inline struct proc*
myproc()
{
    irq_pushoff();
    struct cpu* c = mycpu();
    struct proc* p = c->proc;
    irq_popoff();
    return p;
}


void            proc_init();
void            test_proc_init(uint64 test_func);
int             alloc_pid();
struct proc*    alloc_proc();
void            sleep(void* chan);
void            wakeup(void* chan);
void            exit(int status);
int             kill(int pid);


#endif // __PROC_H__