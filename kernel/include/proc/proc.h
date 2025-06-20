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


/***************** Syscalls flags ******************/

#define CLONE_VM             0x00000100  // 共享地址空间 (线程)
#define CLONE_FS             0x00000200  // 共享文件系统信息
#define CLONE_FILES          0x00000400  // 共享文件描述符表
#define CLONE_SIGHAND        0x00000800  // 共享信号处理程序
#define CLONE_THREAD         0x00010000  // 同线程组 (POSIX 线程)
#define CLONE_SYSVSEM        0x00040000  // 共享 System V 信号量
#define CLONE_SETTLS         0x00080000  // 设置 TLS (必须)
#define CLONE_PARENT_SETTID  0x00100000  // 将子线程 TID 写入 ptid
#define CLONE_CHILD_CLEARTID 0x00200000  // 子线程退出时清除 ctid
#define CLONE_CHILD_SETTID   0x01000000  // 将子线程 TID 写入 ctid
#define CLONE_SIGCHLD        0x00000011  // 子进程退出发送 SIGCHLD

#endif // __PROC_H__