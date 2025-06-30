#ifndef __PAGE_H__
#define __PAGE_H__

#include <common.h>
#include <arch.h>

#pragma pack(push, 1)

struct page {
    uint order : 4;
    uint flag : 4;
    uint8 cnt;
};

#pragma pack(pop)

// 分配在 end 之后，管理所有物理页的 page 数组
// 通过次知道分配器分配的大小，以及该页由多少映射
extern struct page* pages;

#define GET_PAGE(addr) (pages + (((uint64)addr - (uint64)pages) >> PGSHIFT))
#define PAGES_BASE ((struct page *) end)

// 原子增加 pa 对应页的映射
int page_ref_inc(uint64 pa);
// 原子减少 pa 对应页的映射
int page_ref_dec(uint64 pa);

// pagetable 的封装
// cnt 表示引用计数器，表示有多少进程共享此页表
typedef struct {
    pagetable_t pgtbl;
    volatile int cnt;
} upagetable;


#define UPGTBL(upgtbl) (upgtbl->pgtbl)

// 分配一个页表所需的空间，返回该空间的指针
pagetable_t alloc_pagetable();

// 初始化 upagetable
upagetable* upgtbl_init(pagetable_t pagetable);
// 用于 clone，将 upgtbl 的引用计数 +1，返回其本身
upagetable* upgtbl_clone(upagetable* upgtbl);
// 原子增加引用计数
int upgtbl_incr(upagetable* upgtbl);
// 原子减少引用计数
int upgtbl_decr(upagetable* upgtbl);

#endif