#ifndef __VMA_H__
#define __VMA_H__

#include <common.h>
#include <fs/file.h>

struct vm_area {
    uint64 start;
    uint64 end;
    int prot;
    int flags;
    struct file *file;
    off_t offset;
    struct vm_area *next;
    struct vm_area *prev;
    int refcnt;
};

/**
 * 找到进程空间中可以用来分配 vma 的，满足 length 大小的空间
 * 如果没有则扩大 mmap 空间的范围
 * @param p 进程结构体
 * @param length 需要 mmap 的大小
 * @return 返回可映射空间的起始地址
 */
uint64 find_free_vma_range(struct proc *p, size_t length);

/**
 * 寻找虚拟地址处于哪个 vma 范围内
 * @param p 进程结构体
 * @param va 查找的虚拟地址
 * @return va 所在的 vma，若没有返回 NULLL
 */
struct vm_area* find_vma(struct proc* p, uint64 va);

#endif