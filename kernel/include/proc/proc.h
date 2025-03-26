
struct proc {
    int pid;
    volatile int state;

    uint64 sz;

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
};

#ifdef __PROC_C__
struct cpu cpus[NCPU];
#else
extern struct cpu cpus[NCPU];
#endif

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