#include <common.h>
#include <drivers/virtio-mmio.h>
#include <mm/memlayout.h>
#include <mm/mm.h>
#include <klib.h>
#include <proc/proc.h>
#include <platform.h>
#include <trap/trap.h>
#include <debug.h>
#include <arch.h>
#include <syscall.h>

extern char etext[];
extern char end[];
extern char trampoline[];
extern char *init_stack_top;

pagetable_t kernel_pagetable;

void
kinit()
{
    kmem_init((uint64)end, PHYSTOP);
}

void 
kvminit()
{
    kernel_pagetable = kvmmake();
    // mappages(kernel_pagetable, KSTACK(0), (uint64)init_stack_top, PGSIZE, PTE_R | PTE_W);
}


// set pagetable and enable paging
void
kvminithart()
{
    w_satp(MAKE_SATP(kernel_pagetable));
    sfence_vma();

    // init_stack is in rodata
    // whose va mapped to the same pa
}


pagetable_t
kvmmake()
{
    pagetable_t kpgtbl = alloc_pagetable();

    // uart0
    mappages(kpgtbl, UART0, UART0, VIRT_UART0_SIZE, PTE_R | PTE_W);

    // virtio
    mappages(kpgtbl, VIRTIO0, VIRTIO0, VIRTIO_MMIO_DEV_NUM*VIRT_VIRTIO_SIZE, PTE_R | PTE_W);

    // CLINT
    mappages(kpgtbl, CLINT, CLINT, VIRT_CLINT_SIZE, PTE_R | PTE_W);

    // PLIC
    mappages(kpgtbl, PLIC, PLIC, VIRT_PLIC_SIZE, PTE_R | PTE_W);

    // kernel .text
    mappages(kpgtbl, KERNELBASE, KERNELBASE, (uint64)etext - KERNELBASE, PTE_R | PTE_X);

    // kernel data and physical memory space
    mappages(kpgtbl, (uint64)etext, (uint64)etext, PHYSTOP - (uint64)etext, PTE_R | PTE_W);

    // map trampoline page
    mappages(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

    uint64* test_space = (uint64*) kalloc(PGSIZE);
    *test_space = 0x11451419198100UL;
    mappages(kpgtbl, TEST_SPACE, (uint64)test_space, PGSIZE, PTE_R | PTE_COW);

    return kpgtbl;
}


// find the pte's address of given va
// when pte in L2, L1 is NULL and alloc is set
// walk will alloc pagetable
// if alloc == 0 or kalloc failed, return NULL
pte_t*
walk(pagetable_t pgtbl, uint64 va, int alloc)
{
    va >>= 12;
    for (int shift = 18; shift > 0; shift -= 9) {
        int idx = (va >> shift) & 0x1ff;
        pte_t* pte = pgtbl + idx;
        if ((*pte & PTE_V) == 0) {
            if (alloc && (*pte = PA2PTE(alloc_pagetable())) != 0)
                *pte |= PTE_V;
            else return 0;
        } 
        pgtbl = (pagetable_t) PTE2PA(*pte);
    }

    return (pgtbl + (va & 0x1ff));
}


// void
// mappages(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 flags)
// {
//     uint64 start_va = PGROUNDDOWN(va);
//     uint64 end_va = PGROUNDUP(va + sz - 1);
//     int npages = (end_va - start_va) >> PGSHIFT;
//     pte_t *pte = walk(pgtbl, va, WALK_ALLOC);
//     assert(pte);
//     int nr_mapped = 0;

//     while (nr_mapped++ < npages) {
//         *pte++ = PA2PTE(pa) | flags | PTE_V;
//         pa += PGSIZE;
//         // if this is the last pte in L0 pgtbl, start from another pgtbl
//         if (IS_PGALIGNED(pte)) {
//             pte = walk(pgtbl, va + nr_mapped * PGSIZE, WALK_ALLOC);
//             assert(pte);
//         }
//     }
// }


// Look up va in given pgtbl
// return its pa or NULL if not mapped
uint64
walkaddr(pagetable_t pgtbl, uint64 va)
{
    if (va >= MAXVA)
        return 0;
    if (pgtbl == kernel_pagetable)
        return va;

    pte_t* pte = walk(pgtbl, va, WALK_NOALLOC);

    if (!CHECK_PTE(pte, PTE_V | PTE_U))
        return 0;

    uint64 offset = va & (PGSIZE - 1);
    uint64 pa = (uint64) PTE2PA(*pte) | offset;

    return pa;
}


// make user pagetable
// user_pa: address of a 2 PGSIZE space
pagetable_t
uvmmake(uint64 trapframe)
{
    pagetable_t upgtbl = alloc_pagetable();

    // map TRAMPOLINE
    mappages(upgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

    // map TRAPFRAME
    mappages(upgtbl, TRAPFRAME, trapframe, PGSIZE, PTE_R | PTE_W);

    return upgtbl;
}


// just for init proc
pagetable_t
uvminit(uint64 trapframe, const char* init_code, int sz)
{
    assert(init_code);
    assert(sz <= 16*PGSIZE);
    
    void* userspace = kalloc(sz);
    // memset(userspace, 0, sz);
    memmove(userspace, init_code, sz);

    pagetable_t upgtbl = uvmmake(trapframe);

    mappages(upgtbl, 0, (uint64)userspace, sz, PTE_U | PTE_R | PTE_X | PTE_W);

    // map guard page, for uvmcpoy
    mappages(upgtbl, 16*PGSIZE, 0, PGSIZE, 0);

    char* ustack = kalloc(PGSIZE);
    mappages(upgtbl, 17 * PGSIZE, (uint64)ustack, PGSIZE, PTE_U | PTE_R | PTE_W);

    return upgtbl;
}


void 
map_stack(pagetable_t pgtbl, uint64 stack_va) 
{
    void* stack = kalloc(KSTACK_SIZE);
    Assert(stack, "out of memory");
    mappages(pgtbl, stack_va, (uint64)stack, KSTACK_SIZE, PTE_R | PTE_W);
}

uint64
virt_to_phys(uint64 va) {
    return va;
}

uint64
phys_to_virt(uint64 pa) {
    return pa;
}


#define CHECK(cond, msg, ...) \
    if (!(cond)) { \
        Log(ANSI_FG_RED, msg, ## __VA_ARGS__); \
        p->killed = 1; \
        return; \
    }

void 
page_unmap_handler()
{
    struct proc* p = myproc();
    uint64 badv = r_stval();
    uint64 va = PGROUNDDOWN(badv);

    struct vm_area* vma = find_vma(p, va);
    CHECK(vma, "vma not found");

    uint64 scause = r_scause();
    CHECK((scause == STORE_AMO_ACCESS_FAULT && (vma->prot & PROT_WRITE)) && (scause == LOAD_ACCESS_FAULT && (vma->prot & PROT_READ)), "vma prot error");

    char* mem = kalloc(PGSIZE);
    CHECK(mem, "out of memory");
    memset(mem, 0, PGSIZE);

    if (vma->file) {
        file_put(vma->file);
        // file system help
        // ilock(vma->file->ip);
        // readi(vma->file->ip, 0, (uint64)page, 
        //       vma->offset + (va - vma->start), PGSIZE);
        // iunlock(vma->file->ip);
    }

    uint perm = PTE_U;
    perm |= ((vma->prot & PROT_READ) ? PTE_R : 0);
    perm |= ((vma->prot & PROT_EXEC) ? PTE_X : 0);
    perm |= ((vma->prot & PROT_WRITE) ? PTE_W : 0);

    if (vma->flags & MAP_PRIVATE) {
        perm &= ~PTE_W;
        perm |= PTE_COW;
    }
    else if (vma->flags & MAP_SHARED) {
        perm |= PTE_G;
    }

    mappages(UPGTBL(p->pagetable), va, (uint64)mem, PGSIZE, perm);

    flush_tlb_one(va);
}

void free_pgtbl(pagetable_t pgtbl, uint64 sz) {
    uvmunmap(pgtbl, TRAPFRAME, 1, UVMUNMAP_FREE);
    uvmunmap(pgtbl, TRAMPOLINE, 1, UVMUNMAP_NOFREE);
    uvmfree(pgtbl, sz);
}
