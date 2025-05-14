#include <common.h>
#include <klib.h>
#include <debug.h>

#ifdef ARCH_RISCV
#include <riscv.h>
#elif defined(ARCH_LOONGARCH)
#include <loongarch.h>
#endif

#include <irq/interrupt.h>
#include <trap/context.h>
#include <proc/proc.h>
#include <trap/trap.h>
#include <proc/sched.h>
#include <mm/mm.h>
#include <mm/memlayout.h>
#include <fs/file.h>

extern struct proc* proc_list;

volatile int next_pid = 1;
struct proc* init_proc = NULL;

// dead loop
#ifdef ARCH_RISCV
char init_code[] = {0x67, 0x00, 0x00, 0x00};
#elif defined(ARCH_LOONGARCH)
char init_code[] = {0x58, 0x00, 0x00, 0x00};
#endif

int
alloc_pid()
{
    return next_pid++;
}


// alloc a new proc and initialize it
// which can be sched after init
// alloc pid, kstack, trapframe
struct proc*
alloc_proc()
{
    KALLOC(struct proc, p);
    
    p->pid = alloc_pid();
    p->stack = KSTACK(p->pid);

    map_stack(kernel_pagetable, p->stack);

    p->state = INIT;
    p->killed = 0;

    p->trapframe = (struct trapframe*) kalloc(sizeof(struct trapframe));
    Assert(p->trapframe, "out of memory");

    memset(&p->context, 0, sizeof(struct context));
    // leave space for pt_regs
    context_set_stack(p, p->stack + KSTACK_SIZE);
    context_set_init_func(p, (uint64) dive_to_user);
    
    p->next = NULL;
    p->parent = NULL;

    p->cwd = "/";

    #ifdef ARCH_RISCV
    p->fdt = (struct files_struct*) kalloc(sizeof(struct files_struct));
    Assert(p->fdt, "out of memory");
    fdt_init(p->fdt, "fdt_lock");
    #endif

    return p;
}


void
proc_init()
{
    struct proc* p = alloc_proc();

    // user vm space init
    p->pagetable = uvminit((uint64)p->trapframe, init_code, sizeof(init_code));
    p->sz = 2*PGSIZE;

    trapframe_set_era(p, 0);
    trapframe_set_stack(p, 3*PGSIZE);
    // p->trapframe->epc = 0;
    // p->trapframe->sp = 3*PGSIZE;

    strcpy(p->name, "init");
    
    p->state = RUNNABLE;

    proc_list = p;
    init_proc = p;

    // should be deleted
    // cpus[0].proc = p;
}

void 
test_proc_init(uint64 test_func)
{
    struct proc* test_proc = alloc_proc();
    context_set_init_func(test_proc, test_func);

    strcpy(test_proc->name, "test");
    test_proc->state = RUNNABLE;

    proc_list->next = test_proc;
}


void
sleep(void* chan)
{
    struct proc *p = myproc();
    // debug("sleep on chan %p", chan);
    p->chan = chan;
    p->state = SLEEPING;

    sched();

    p->chan = NULL;
}


// wake up all process sleeping on the chan
// P.S. current NCPU = 1, so no need to worry about Lost Wakeup
void
wakeup(void* chan) 
{
    struct proc* cur = myproc();
    // debug("wakeup on chan %p", chan);

    for (struct proc* p = proc_list; p; p = p->next) {
        // p != myproc()
        if (p != cur) {
            if (p->state == SLEEPING && p->chan == chan)
                p->state = RUNNABLE;
        }
    }
}


// Exit current process
void
exit(int status)
{
    struct proc* p = myproc();
    // close opened files

    // reparent to init

    // wakeup parent, which might be sleeping in wait

    // set status ZOMBIE
    p->status = status;
    p->state = ZOMBIE;

    // swtch to scheduler, should never return
    sched();
    panic("proc %s should be exited", p->name);
}


// kill process by pid
// but not kill it right now
int
kill(int pid)
{
    for (struct proc* p = proc_list; p; p = p->next) {
        if (p->pid == pid) {
            p->killed = 1;
            // if this proc is sleeping, wake it up
            p->state = (p->state == SLEEPING ? RUNNABLE : p->state);

            return 0;
        }
    }

    // not found
    return -1;
}

#ifdef ARCH_RISCV
// Create new process, copying form parent
int
fork()
{
    struct proc* p = myproc();
    struct proc* cp = alloc_proc();
    assert(cp);

    cp->pagetable = uvmmake((uint64)p->trapframe);

    // copy user pagetable from p
    // COW later
    uvmcopy(cp->pagetable, p->pagetable, p->sz);

    // copy trapframe
    *(cp->trapframe) = *(p->trapframe);
    // let fork return 0 in child proc
    trapframe_set_return(cp, 0);

    // files

    cp->parent = p;
    cp->state = RUNNABLE;

    return cp->pid;
}
#endif
