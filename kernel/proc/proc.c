#include <defs.h>
#include <debug.h>

extern char *init_stack_top;

struct proc* proc_list;
int next_pid = 1;
struct proc* init_proc;

struct cpu cpus[NCPU];

char initcode[] = {
    
};


// Prepare resources for the first process
void
proc_init()
{
    struct proc* p = alloc_proc();
    init_proc = p;

    uvminit(p->pagetable, initcode, sizeof(initcode));
    p->sz = PGSIZE;
    p->trapframe->epc = 0;
    p->trapframe->sp = PGSIZE;

    p->state = RUNNABLE;

    strcpy(p->name, "initcode");
}

// alloc a new process and initialize it
// return the new process, 
// which runs in kernel mode first and then jumps to user mode
struct proc*
alloc_proc() 
{
    struct proc* p;
    p = kalloc(sizeof(struct proc));
    Assert(p, "Memory allocation failed");
    memset(p, 0, sizeof(struct proc));

    // alloc trapframe space
    p->trapframe = (struct trapframe*) kalloc(sizeof(struct trapframe));
    Assert(p->trapframe, "Memory allocation failed");

    // make pagetable for the new process
    uvmmake(p);

    p->context.ra = (uint64) usertrapret;
    p->context.sp = (uint64) init_stack_top;

    p->pid = alloc_pid();
    p->state = INIT;

    return p;
}

static int 
alloc_pid()
{
    return next_pid++;
}


// when use this function
// Interrupts must be disabled
struct cpu* 
mycpu()
{
    int id = r_tp();
    return &cpus[id];
}


// Per-CPU process scheduler
// Each CPU calls scheduler() after settinf it up
// Scheduler never return, just looping, doing:
// - choose a proc to run
// - swtch to start running the choosen proc
// - eventually that process transfer control, via setch() back to this scheduler()
void
scheduler()
{
    struct proc* p;
    struct cpu* c = mycpu();
    c->proc = NULL;
    for (;;) {
        // Avoid deadlock by ensuring that devices can interrupt
        intr_on();

        for (p = proc_list; p; p = p->next) {
            if (p->state == RUNNABLE) {
                p->state = RUNNING;
                c->proc = p;
                swtch(&c->context, &p->context);

                // Process is done
                // It should have changed its p->state before coming back
                c->proc = NULL;
            }
        }
    }
    panic("No runnable process");
}
