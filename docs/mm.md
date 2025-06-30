# 内存管理模块 mm

HANA 的内存管理模块（Memory Management, MM 提供了从物理内存分配、页表管理到用户进程地址空间映射的完整支持。该模块实现了内核态和用户态之间的内存隔离、共享、拷贝等功能，并支持现代操作系统所需的虚拟内存机制。

<br/>

## 1. 内存分配与释放接口

`void kmem_init(uint64 va_start, uint64 va_end)`

初始化内核内存分配器（slab 或 buddy system）；

- `va_start`：可分配内存起始虚拟地址；
- `va_end`：可分配内存结束地址；

<br/>

`void kinit()`

封装 `kmem_init()`，用于不同架构选择合适的内存分配范围；

<br/>

`void* kalloc(uint64 sz)`

分配一段物理连续、大小为 `sz` 的内存；

<br/>

`void* kcalloc(uint64 nr, uint64 sz)`

分配 `nr` 块，每块大小为 `sz` 的内存，并初始化为 0；

<br/>

`void kfree(void *addr)`

释放由 `kalloc` 分配的内存，`addr` 必须是 `kalloc` 返回的地址。

<br/>

<br/>

## 2. 页表操作接口

`pagetable_t kvmmake()`

创建并初始化一个内核页表，完成内核必要的映射，返回初始化后的页表指针；

<br/>

`pte_t* walk(pagetable_t pgtbl, uint64 va, int alloc)`

查找指定虚拟地址对应的页表项，返回找到的页表项指针，若无且 `alloc=0` ，返回 NULL。

- `pgtbl`：目标页表；
- `va`：虚拟地址；
- `alloc`：是否自动分配缺失的页目录；

<br/>

`void mappages(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 flags)`

将虚拟地址 `va` 到 `va+sz` 映射到物理地址 `pa`，支持跨页

flags 在各架构由 `PTE_U` `PTE_RONLY` `PTE_RX` `PTE_RW` 的宏

<br/>

`void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)`
**作用**：解除虚拟地址 `va` 开始的 `npages` 页映射；**参数**：`do_free=1`：释放物理页；`do_free=0`：不解绑物理页。

<br/>

`uint64 walkaddr(pagetable_t pgtbl, uint64 va)`

查找虚拟地址在页表中映射的物理地址，找不到则返回 0。

<br/>

<br/>

## 3. 用户空间映射与拷贝

`pagetable_t uvmmake(uint64 trapframe)`

新建用户页表，仅映射 trapframe，RISC-V 需额外映射 TRAMPOLINE 区域。返回映射后的页表

<br/>

`pagetable_t uvminit(uint64 trapframe, const char* init_code, int sz)`

初始化用户页表并返回。映射了：

- trapframe
- 用户栈
- 初始化代码（initcode）

<br/>

`void uvmcopy(pagetable_t cpgtbl, pagetable_t ppgtbl, uint64 sz)`

复制父进程的用户空间（代码、堆、栈）到子进程。`clone()` 中使用。

<br/>

`void map_stack(pagetable_t pgtbl, uint64 stack_va)`

为进程分配并映射用户栈

`stack_va` ：栈空间底部的虚拟地址

<br/>

`int copyout(pagetable_t pgtbl, uint64 dstva, void* src, size_t len)`

将内核空间的数据拷贝到用户空间。返回 0，失败返回 -1。

<br/>

`int copyin(pagetable_t pagetable, char* dst, uint64 srcva, size_t len)`

将用户空间的数据拷贝到内核空间，返回实际拷贝字节数，失败返回 -1。

<br/>

`size_t copyinstr(pagetable_t pagetable, char* dst, uint64 srcva, size_t max)`

将用户空间字符串拷贝到内核空间，以 '\0' 结尾。成功则返回长度，失败返回 -1。

<br/>

<br/>

## 4. 虚实地址转换接口

`uint64 virt_to_phys(uint64 va)`

将内核虚拟地址转换为物理地址

`uint64 phys_to_virt(uint64 pa)`

将物理地址转换为内核虚拟地址；

<br/>

<br/>

## 5. 虚拟内存管理函数

`void free_pgtbl(pagetable_t pgtbl, uint64 sz)`

释放页表从虚拟地址 0 到 `sz` 的所有映射，还会释放 TRAMPOLINE 和 trapframe 映射。

<br/>

`void freewalk(pagetable_t pgtbl, int level)`

递归释放页表自身占据的空间。

`level` 表示当前处理的是第几级页表。

<br/>

`void uvmfree(pagetable_t pgtbl, uint64 sz)`

释放页表从虚拟地址 0 到 `sz` 的所有映射

<br/>

`int do_munmap(void* addr, size_t length)`

实现 `munmap` 系统调用，成功返回 0，失败返回 -1。

<br/>

<br/>

## 6. 异常处理

`void store_page_fault_handler()`

实现 Store 类型页错误的处理，辅助实现写时复制（Copy-on-Write, COW）机制。

`void page_unmap_handler()`

处理缺页异常，辅助实现 mmap COW；

<br/>

<br/>

## 示例代码片段

### 创建用户进程页表：

```c_cpp
pagetable_t user_pgtbl = uvmmake((uint64) myproc()->trapframe);
```

### 映射用户栈：

```c_cpp
map_stack(user_pgtbl, USER_STACK_TOP);
```

### 拷贝用户字符串：

```c_cpp
char buf[256];
copyinstr(myproc()->pagetable, buf, user_str_addr, sizeof(buf));
```