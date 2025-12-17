#ifndef __MM_H__
#define __MM_H__

#include <klib.h>
#include <arch.h>

extern pagetable_t kernel_pagetable;

struct vm_area;

/**
 * 内核分配器 slab, buddy system 初始化
 * @param va_start 可分配空间的起点
 * @param va_end 可分配空间的终点
 */
void        kmem_init(uint64 va_start, uint64 va_end);

/**
 * kmem_init 的 wrapper，各个架构需要自己选择可分配空间范围
 */
void        kinit();

/**
 * 内核分配器
 * 分配在物理上连续的，大小大于等于 sz 的空间
 * @param sz 需要分配的空间大小（字节）
 * @return 内核分配空间的起始虚拟地址
 */
void*       kalloc(uint64 sz);

/**
 * kalloc 的封装实现
 * 分配在物理上连续的，大小大于等于 nr*sz，且初始化全为 0 的空间
 * @param nr 要分配的块的数量
 * @param sz 每一块的大小（字节）
 * @return 内核分配空间的起始虚拟地址
 */
void*       kcalloc(uint64 nr, uint64 sz);

/**
 * 释放由 kalloc 分配的空间，大小由 kfree 自行判定
 * @param addr 由 kalloc 分配的空间的起始虚拟地址
 */
void        kfree(void *addr);

/**
 * 初始化内核页表，并写入全局变量 kernel_pagetable
 * loongarch 还会配置页表相关的设置
 */
void        kvminit();

/**
 * 将内核页表写入加对应的寄存器
 * loongarch 还会调用 tlb_init 初始化 TLB
 */
void        kvminithart();

/**
 * 初始化内核页表，完成内核态必要的映射
 * @return 一个完成初步映射的内核页表
 */
pagetable_t kvmmake();

/**
 * 遍历页表，找到指定虚拟地址对应的页表项的地址
 * @param pgtbl 要遍历的页表
 * @param va 要查找的虚拟地址
 * @param alloc 是否分配的标识符。在页表中没有找到 va 的 pte 时，若 alloc=1，walk 会自动完成分配， 若为 0, 则此时返回 NULL
 * @return 页表中 va 对应的页表项地址。若没有找到且 alloc = 0，则返回 NULL
 */
pte_t*      walk(pagetable_t pgtbl, uint64 va, int alloc);

/**
 * 在页表中映射虚拟空间与物理空间
 * @param pgtbl 完成映射的页表
 * @param va 需要映射的虚拟地址
 * @param pa 与之对应的物理地址，要求与 va 有着相同的页内偏移量
 * @param sz 需要映射的空间的大小
 * @param flags 映射空间的权限
 */
void        mappages(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 flags);

/**
 * @param pgtbl 页表
 * @param va 要在页表中查找的虚拟地址
 * @return va 在 pgtbl 里面所映射的*物理地址*
 */
uint64      walkaddr(pagetable_t pgtbl, uint64 va);

/**
 * 部分初始化用户页表，映射 trapframe。riscv 需要额外映射 TRAMPOLINE
 * @param trapframe trapframe 的内核态虚拟地址，可以在函数内转化为物理地址
 * @return 初始化后的用户页表
 */
pagetable_t uvmmake(uint64 trapframe);

/**
 * 初始化内核页表，将 initcode, 用户栈，trapframe 等必要的区域全部映射
 * @param trapframe trapframe 的内核态虚拟地址，可以在函数内转化为物理地址
 * @return 初始化后的用户页表
 */
pagetable_t uvminit(uint64 trapframe, const char* init_code, int sz);

/**
 * 拷贝用户空间代码，数据，栈和堆，用于 clone()
 * @param cpgtbl 子进程页表
 * @param ppgtbl 父进程页表
 * @param sz 用户空间代码，数据，栈和堆的顶部
 */
void        uvmcopy(pagetable_t cpgtbl, pagetable_t ppgtbl, uint64 sz);
void        uvmcopy_alloc(pagetable_t cpgtbl, pagetable_t ppgtbl, struct vm_area *vma_list);

/**
 * 给定栈区域的起始位置的虚拟地址，自动分配物理地址，然后在页表完成映射
 * @param pgtbl 页表
 * @param stack_va 栈底虚拟地址
 */
void map_stack(pagetable_t pgtbl, uint64 stack_va);

/**
 * 将内核空间的内存拷贝到用户空间
 * @param pgtbl 页表
 * @param dstva 用户空间的目的地址
 * @param src 内核空间的源地址
 * @param len 拷贝的字节数
 * @return 成功返回 0, 失败 -1
 */
int         copyout(pagetable_t pgtbl, uint64 dstva, void* src, size_t len);

/**
 * 将用户空间的内存拷贝到内核空间
 * @param pagetable 页表
 * @param dst 内核空间的目的地值
 * @param srcva 用户空间的源地址
 * @param len 要拷贝的字节数
 * @return 实际拷贝的字节数。如果 dst == NULL，返回 -1
 */
int         copyin(pagetable_t pagetable, char* dst, uint64 srcva, size_t len);

/**
 * 将用户空间的字符串拷贝到内核空间，遇到 0 截止
 * @param pagetable 页表
 * @param dst 内核空间的目的地值
 * @param srcva 用户空间的源地址
 * @param len 要拷贝的字节数
 * @return 实际拷贝的字节数。地址翻译失败，或者没有遇到 '\0'，返回 -1
 */
size_t      copyinstr(pagetable_t pagetable, char* dst, uint64 srcva, size_t max);

/**
 * 内核态虚实地址翻译
 * @param va 内核虚拟地址
 * @return 对应的物理地址
 */
uint64      virt_to_phys(uint64 va);

/**
 * 内核态虚实地址翻译
 * @param pa 内核物理地址
 * @return 对应的虚拟地址
 */
uint64      phys_to_virt(uint64 pa);

/**
 * mappages 的反向操作，解除从 va 开始 npages 页的映射，可以通过 do_free 决定是否释放物理页
 * @param pagetable 页表
 * @param va 虚拟地址，要求页对齐
 * @param npages 从 va 开始解除映射的页的数量
 * @param do_free 如果为 1,则释放对应的物理页；为 0 则不释放
 */
void        uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free);

/**
 * 将页表从虚拟空间 0-sz 范围内的页解除映射，并且释放 TRAMPOLINE 与 trapframe 映射（如果有）
 * @param pgtbl 页表
 * @param sz 决定释放的空间范围
 */
void free_pgtbl(pagetable_t pgtbl, struct vm_area *vma);

/**
 * 递归地释放页表自身占据的空间
 * @param pgtbl 释放的页表
 * @param level 表示释放的是第几级页表
 */
void        freewalk(pagetable_t pgtbl, int level);

/**
 * 将页表从虚拟空间 0-sz 范围内的页解除映射
 * @param pgtbl 页表
 * @param sz 决定释放的空间范围
 */
void        uvmfree(pagetable_t pgtbl, uint64 sz);

void        uvmfree_vma(pagetable_t pgtbl, struct vm_area* vma_list);

/**
 * munmap 系统调用的实际实现
 * @param addr munmap 地址起始点
 * @param length munmap 范围大小
 * @return 成功返回 0，失败返回 -1
 */
int         do_munmap(void* addr, size_t length);

/**
 * store page fault 的异常处理函数，辅助实现 COW 机制
 */
void        store_page_fault_handler();

/**
 * 缺页的异常处理函数，实现辅助实现 COW
 */
void        page_unmap_handler();

static inline uint64 phys_page_number(uint64 pa) {
    return pa >> PGSHIFT;
}

#include <mm/page.h>

ssize_t     copy_from_user(void *to, const void *from, size_t n);
ssize_t     copy_to_user(void *to, const void *from, size_t n);
ssize_t     copy_from_user_str(char* to, const void* from, size_t max);
ssize_t     copy_to_user_str(void* to, const char* from, size_t max);

#define IS_DATA(addr) (((addr) >= (uint64) pages) && ((addr) < PHYSTOP)) 

#endif // __MM_H__