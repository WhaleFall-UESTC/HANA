#ifndef __PROC_H__
#define __PROC_H__

#include <trap/context.h>
#include <trap/trap.h>
#include <irq/interrupt.h>
#include <mm/mm.h>
#include <fs/path.h>

struct proc {
    int pid;                        // 进程 id
    int tgid;                       // 进程组 id
    volatile int state;             // 进程的状态

    uint64 tls;                     // Thread Local Storage

    uint64 heap_start;              // 堆开始的地方
    uint64 sz;                      // brk

    // pagetable_t pagetable;
    upagetable* pagetable;          // 用户态页表
    uint64 stack;                   // 内核栈的虚拟地址，指向栈区域的顶部
    struct trapframe* trapframe;    // trapframe 指针 
    struct context context;         // 进程的上下文

    struct files_struct* fdt;       // 进程持有的文件
    struct path cwd;              // 当前目录
    struct path root;             // 进程运行的根目录

    int killed;                     // 进程是否被 kill 的标志
    void* chan;                     // 进程正在 sleep 的时间
    long sleeping_due;              

    int status;                     // exit() return status

    // wait4 中用于标识进程的状态
    int stopped;
    int continued;
    int exit_signal;
    int waited;

    struct proc* parent;            // 父进程指针

    struct proc* next;              // 进程链表的双向值镇
    struct proc* prev;

    struct vm_area* vma_list;       // vm_area 链表
    uint64 mmap_base;               // mmap 基地址
    uint64 mmap_brk;                // mmap 范围的顶部

    // clone 的标记
    int cleartid;
    int sigchld;
    
    char name[16];                  // 进程名字

    uint64 utime;
    uint64 stime;
};

enum proc_state{ INIT, SLEEPING, RUNNABLE, RUNNING, ZOMBIE, NR_PROC_STATE };

// 存储所有进程的链表
extern struct proc* proc_list;

struct cpu { 
    struct context context;   // cpu 保存的上下文
    struct proc* proc;        // 当前 CPU 运行的进程
    int noff;                 // 中断嵌套计数
    int intena;               // irq_pushoff 前的中断使能标志
};

// cpu 数组，通过 cpuid 获得自身的结构体
extern struct cpu cpus[NCPU];

#define CPUID(c) (int)(((uint64)((c) - cpus))/sizeof(struct cpu))

/**
 * 获取当前核的 cpu 结构体
 * @return: 当前 cpuid 对应的 cpu 结构体指针
 */
static inline struct cpu*
mycpu() 
{
    return &cpus[r_cpuid()];
}

/**
 * 获取当前核正在运行的进程
 * @return: 当前正在运行的进程的指针
 */
static inline struct proc*
myproc()
{
    irq_pushoff();
    struct cpu* c = mycpu();
    struct proc* p = c->proc;
    irq_popoff();
    return p;
}

#include <mm/vma.h>

/**
 * 初始化 init 进程，使之完成被调度的准备
 * 并将其放入 proc_list 的中，等待被调度
 */
void            proc_init();

/**
 * 初始化 test 进程，调度后不可被抢占，用作内核态的代码的测试
 */
void            test_proc_init(uint64 test_func);

/**
 * 获取系统分配的 pid
 * @return: 一个新的进程 id
 */
int             alloc_pid();

/**
 * 分配一个进程结构体，初始化其内核栈，trapframe，最初执行的函数与文件
 * 不将其余的部分初始化为 0
 * @return 分配好的进程指针
 */
struct proc*    alloc_proc();

/**
 * 将进程设置为 SLEEPING 状态，让出 CPU，直到等待的事件完成调用 wakeup 后，恢复可被调用的状态
 * @param chan 正在等待的事件
 */
void            sleep(void* chan);

/**
 * 将所有正在等待指定事件的进程唤醒，使之可被重新调度
 * @param chan 要唤醒的事件
 */
void            wakeup(void* chan);

/**
 * 退出当前的进程
 * @param status 进程退出状态
 */
void            do_exit(int status);

/**
 * 杀死指定的进程
 * @param pid 要杀死的进程的 pid
 */
int             kill(int pid);

/**
 * 将指定进程的子进程托管到 init 进程，使 init 成为新的父进程
 * @param p 要托管的进程的结构体
 */
void            reparent(struct proc* p);

/**
 * 释放当前进程所占有的内存空间
 * @param p 要释放的进程的结构体
 */
void            freeproc(struct proc* p);

/**
 * 释放进程的页表中所有已分配的内存，包括页表自身占有的内存
 * @param p 要释放的页表所对应的进程的结构体
 */
void            proc_free_pagetable(struct proc* p);


#define EXIT_IF(cond, msg, ...) \
    if (cond) { \
        Log(ANSI_FG_RED, msg, ## __VA_ARGS__); \
        do_exit(-1); \
    }

#define PROC_SCHED 

#endif // __PROC_H__