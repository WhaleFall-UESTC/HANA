#ifndef __PROC_H__
#define __PROC_H__

#include <trap/context.h>
#include <trap/trap.h>
#include <irq/interrupt.h>
#include <mm/mm.h>

struct proc {
    int pid;
    int tgid;
    volatile int state;
    uint64 tls;

    uint64 sz;

    // pagetable_t pagetable;
    upagetable* pagetable;
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

    int cleartid;
    int sigchld;
    
    char name[16];

    uint64 utime;
    uint64 stime;
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


#define EXIT_IF(cond, msg, ...) \
    if (cond) { \
        Log(ANSI_FG_RED, msg, ## __VA_ARGS__); \
        exit(-1); \
    }



#endif // __PROC_H__