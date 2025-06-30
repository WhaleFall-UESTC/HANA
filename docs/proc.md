# proc

本篇文档讲述 HANA 的进程管理部分，实现了从进程创建、调度、执行、退出到资源回收的完整生命周期管理。它基于类 Unix 的进程模型，支持多任务并发执行，并提供了丰富的系统调用接口供用户程序调用。

<br/>

## 1. 核心数据结构

### struct proc

```C
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
    char* cwd;                      // 当前目录

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
```

<br/>

### struct cpu

```C
struct cpu { 
    struct context context;   // cpu 保存的上下文
    struct proc* proc;        // 当前 CPU 运行的进程
    int noff;                 // 中断嵌套计数
    int intena;               // irq_pushoff 前的中断使能标志
};
```

<br/>

### struct context

loongarch 与 riscv 的实现有所区别，但都是由返回地址寄存器，栈指针，调用者保存寄存器，以及例外前模式信息和例外程序返回地址组成

```C
struct context {
    uint64 ra;
    uint64 sp;
    uint64 s[12];

    uint64 sstatus;
    uint64 epc;
};
```

<br/>

### enum proc_state

进程状态枚举

```C
enum proc_state{ INIT, SLEEPING, RUNNABLE, RUNNING, ZOMBIE, NR_PROC_STATE };
```

<br/>

<br/>

## 2. 进程状态机

```
INIT → RUNNABLE ↔ RUNNING → ZOMBIE
          ↓
       SLEEPING → (wakeup) → RUNNABLE
```

- **RUNNABLE**: 等待调度
- **RUNNING**: 正在运行
- **SLEEPING**: 等待某个事件（如 I/O 完成）
- **ZOMBIE**: 已退出但未被父进程回收

<br/>

<br/>

## 3. 系统调用接口

所有的系统调用功能参考 POSIX 接口实现

 `sys_clone`

```C
SYSCALL_DEFINE5(clone, int, unsigned long, flags, void*, stack, void*, ptid, void*, tls, void*, ctid)
```

`sys_execve`

```C
SYSCALL_DEFINE3(execve, int, const char*, upath, const char**, uargv, const char**, uenvp)
```

`sys_wait4`

```C
SYSCALL_DEFINE3(wait4, int, int, pid, int*, status, int, options)
```

`sys_exit`

```C
SYSCALL_DEFINE1(exit, int, int, ec)
```

`sys_getpid` `sys_getppid`

```C
SYSCALL_DEFINE0(getpid, int)
SYSCALL_DEFINE0(getppid, int)
```

`sys_sched_yield`

```C
SYSCALL_DEFINE0(sched_yield, int)
```

<br/>

<br/>

## 4. 关键函数说明

`struct proc* alloc_proc()`
分配一个新的进程结构体，初始化其内核栈、trapframe、初始执行函数， 文件等。但是由于没有准备用户空间，所以并不处于可被调度的状态

`void proc_init()`
初始化 init 进程，使之完成被调度的准备并将其放入 proc_list 的中，等待被调度。由 main 函数调用

`int alloc_pid()`
获取系统分配的 pid

`void sleep(void* chan)`
将进程设置为 SLEEPING 状态，让出 CPU，直到等待的事件完成调用 wakeup 后，恢复可被调用的状态

`wakeup(void* chan)`
将所有正在等待指定事件的进程唤醒，使之可被重新调度

`reparent(struct proc* p)`
将指定进程的子进程托管到 init 进程，使 init 成为新的父进程

`void freeproc(struct proc* p)`
释放当前进程所占有的内存空间

`void proc_free_pagetable(struct proc* p)`
释放进程的页表中所有已分配的内存，包括页表自身占有的内存

`static inline struct proc* myproc()`
获取当前的 CPU 核正在运行的进程

`static inline struct cpu* mycpu()`
获取当前核的 cpu 结构体

<br/>

<br/>

## 5. 调度机制

HANA 的调度机制是由 `scheduler`，`sched`，`yield` 共同完成

在 scheduler 内部的循环中，调度器会不停地遍历 proc_list，找到一个处于 RUNNABLE 状态的进程，接着调用 swtch 调度它。swtch 会将当前调用者需要保存的寄存器存放在 mycpu() 的结构体中，并将调度的进程的上下文加载，这样，就成功地切换到了选择的进程。

在这个进程经过 yield -> sched ->swtch 的过程，将当前的保存寄存器放在自身的进程结构体的 context 内，并将 mycpu() 里面的寄存器加载出来，就这样回到了 scheduler。

对于 scheduler 来说，这个过程就像是在中途调用了一个名为 swtch的普通函数一样；同理，对于调用 yield 最后 swtch 的进程来说也是这样

`swtch(old, new)`
上下文切换函数，保存当前上下文，恢复目标上下文。

`scheduler()`
调度器主循环，选择下一个 RUNNABLE 进程执行。

`sched() / yield()`
主动让出 CPU，调用 sched() 切换回调度器

<br/>

<br/>

## 6. 退出与回收机制

当进程调用 do_exit() 退出的时候，此时会将进程的状态设置为 ZOMBIE，将进程结构体的 killed 设置为 1，并不会立刻回收资源。等到父进程 wait 的时候，会找到 ZOMBIE 的子进程并调用 freeproc() 回收其资源

父进程退出的时候会调用 reparent 回收其

<br/>

<br/>

## 从 init 进程看 HANA 的进程管理模块

首先在 main 会调用 proc_init 来初始化 init 进程。

proc_init 调用 alloc_proc 得到一个分配好的进程结构体，设置好了其 pid, trapframe 的内核空间， 内核栈，fdt 表，并将 context 初始化，设置 context 的 ra 为 dive_to_user，sp 为 p->stack（内核栈）。如此，这个进程一旦被 swtch 调度，返回后就会来到 dive_to_user 函数，准备进入用户态

随后，proc_init 调用 uvminit 初始化了其内核空间，将准备好的 initcode 映射到了虚拟地址为 0 的位置，并且还映射了用户栈，TRAPMPOLINE 代码（用户态与内核态切换的代码）。riscv 还需要将 trapframe 也 map 上，因为 TRAMPOLINE 需要在没有从用户态页表切换到内核态页表的情况下读取 trapframe，而 loongarch 可以利用 PGDL 与 PGDH 的机制避开这一点。

在完成用户空间的准备之后，init 进程的 trapframe 的例外返回地址（riscv 是 sepc，loongarch 是 era）设置为 0，即 initcode 代码段开始之处；stack 设置为用户栈的栈顶。最后将状态设置为 RUNNABLE 加入 proc_list

main 函数最后会调用 scheduler。调度器选中了目前唯一一个 RUNNABLE 进程 init，并用 swtch 调度它。swtch 将 init 的 context 里面的 ra, sp 加载到了真实的寄存器 ra, sp 上。当 swtch 返回的时候，pc 跳转到了当前的 ra 所指向的地址，即 dive_to_user。在 dive_to_user 的最后进入 userret，将 init trapframe 的 sp 与例外返回地址加载出来。到了最终的 sret/ ertn，CPU 将模式切换到用户态，并将 pc 设置为例外返回地址，也就是我们在 proc_init 里面设置的 0。这样，我们就能开始运行用户态代码。