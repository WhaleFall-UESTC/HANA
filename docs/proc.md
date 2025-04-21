# proc

## proc.c
### struct proc
proc 结构体在 proc.h 中定义
```C
struct proc {
    int pid;                // pid
    volatile int state;     // 进程当前状态

    uint64 sz;              // 用户态已用空间大小，也是分配内存的起始地址

    pagetable_t pagetable;  // 用户态页表，内核态共用 kernel_pagetable
    uint64 stack;           // 内核栈起始地址
    struct trapframe* trapframe;
    struct context context; // 内核态上下文


    int killed;             // 是否被杀了
    void* chan;             // 进程当前 sleep 等待的事件
    int status;             // exit 的返回值

    struct proc* parent;    // 父进程
    struct proc* next;      // 目前 proc 用单向链表管理
    
    char name[16];          // 进程的名字，debug 用
};
```
<br>

### struct proc alloc_proc()
分配一个 proc 结构体，使之完成可以被 scheduler() 调度的准备，但是没有分配用户空间，所以实际上还是不能运行，这也是 INIT 状态的含义
<br><br>
分配了独有的内核栈以及 trapframe，并且设置了 context 的 sp 与 ra 的值，一旦被调度，就会切换到预先准备的内核栈，以及跳到 ra 所存储的函数地址
<br><br>
trapframe 是物理地址；sp 和 stack 都是虚拟地址，不过前者指向栈顶，后者则指向栈空间开始的地方；ra 默认是 dive_to_user 的物理地址，当然可以修改
