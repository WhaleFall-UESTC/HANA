#ifndef __PROC_H__
#define __PROC_H__

struct proc {
    int pid;
    volatile int state;
    int killed;

    int sz;

    pagetable_t pagetable;
    uint64 stack;
    struct trapframe* trapframe;

    struct context context;

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

#ifdef __PROC_C__
struct cpu cpus[NCPU];
#else
extern struct cpu cpus[NCPU];
#endif

#define CPUID(c) (int)(((uint64)((c) - cpus))/sizeof(struct cpu))

static inline struct cpu*
mycpu() 
{
    return &cpus[r_tp()];
}

static inline struct proc*
myproc()
{
    return mycpu()->proc;
}


void            proc_init();
int             alloc_pid();
struct proc*    alloc_proc();

#endif // __PROC_H__