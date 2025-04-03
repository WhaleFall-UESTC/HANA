# proc

## proc.c
### struct proc alloc_proc()
分配一个 proc 结构体，使之完成可以被 scheduler() 调度的准备，但是没有分配用户空间，所以实际上还是不能运行
<br><br>
分配了独有的内核栈以及 trapframe，并且设置了 context 的 sp 与 ra 的值，一旦被调度，就会切换到预先准备的内核栈，以及跳到 ra 所存储的函数地址