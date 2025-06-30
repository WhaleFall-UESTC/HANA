#include <common.h>
#include <mm/memlayout.h>
#include <mm/mm.h>
#include <klib.h>
#include <proc/proc.h>
#include <trap/trap.h>
#include <debug.h>
#include <arch.h>
#include <syscall.h>

extern char end[], trampoline[];
extern void tlb_refill();

// store kernel pagetable physical address
pagetable_t kernel_pagetable;

void
kinit()
{
    kmem_init((uint64)end, RAMTOP);
}


void 
tlbinit()
{
    w_csr_stlbps(PGSHIFT);  //TLB 页大小为 PGSIZE
    w_csr_tlbrehi(PGSHIFT);
    w_csr_asid(0);
    w_csr_tlbrentry((uint64)tlb_refill);    // 注册 TLB 充填异常处理函数
    invtlb();
}

void 
kvminit()
{
    kernel_pagetable = kvmmake();

    // Sv39-like style
    // pagetable config
    w_csr_pwcl((PTBASE | (DIRWIDTH << 5) | (DIRBASE(1) << 10) | (DIRWIDTH << 15) | (DIRBASE(2) << 20) | (DIRWIDTH << 25)) | CSR_PWCL_PTEWidth64);
    w_csr_pwch(0);
    w_csr_rvacfg(8);
}

// set pagetable and enable paging
void
kvminithart()
{
    // csr_write(CSR_PGDH, (uint64) kernel_pagetable);
    w_csr_pgdh(KERNEL_VA2PA(kernel_pagetable));
    tlbinit();
}

pagetable_t 
kvmmake()
{
    pagetable_t kpgtbl = alloc_pagetable();

    // anything else is mapped by DMW0

    mappages(kpgtbl, TRAMPOLINE, KERNEL_VA2PA(trampoline), PGSIZE, PTE_PLV3 | PTE_MAT_CC | PTE_G | PTE_P);

    uint64* test_space = (uint64*) kalloc(PGSIZE);
    *test_space = 0x11451419198100UL;
    mappages(kpgtbl, TEST_SPACE, KERNEL_VA2PA(test_space), PGSIZE, PTE_PLV3 | PTE_MAT_CC | PTE_P | PTE_COW | PTE_D | PTE_NX);

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
    pgtbl = (pagetable_t) KERNEL_PA2VA(pgtbl);
    for (int shift = 18; shift > 0; shift -= 9) {
        int idx = (va >> shift) & 0x1ff;
        pte_t* pte = pgtbl + idx;
        if ((*pte & PAMASK) == 0) {
            if (alloc == WALK_NOALLOC || (*pte = PA2PTE(alloc_pagetable())) == 0)
                return 0;
        } 
        pgtbl = (pagetable_t) KERNEL_PA2VA(PTE2PA(*pte));
    } 

    return (pgtbl + (va & 0x1ff));
}


// void
// mappages(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 flags)
// {
//     uint64 start_va = PGROUNDDOWN(va);
//     uint64 end_va = PGROUNDUP(va + sz - 1);
//     int npages = (end_va - start_va) >> PGSHIFT;
//     pte_t* pte = walk(pgtbl, va, WALK_ALLOC);
//     assert(pte);
//     int nr_mapped = 0;

//     while (nr_mapped++ < npages) {
//         *pte = PA2PTE(pa) | flags | PTE_V;
//         pte++;
//         page_ref_inc(pa);
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
    pgtbl = (pagetable_t) KERNEL_PA2VA(pgtbl);
    pte_t* pte = walk(pgtbl, va, WALK_NOALLOC);

    if ((*pte & PTE_V) == 0)
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
    // mappages(upgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_PLV3 | PTE_MAT_CC | PTE_P);

    // map TRAPFRAME
    mappages(upgtbl, TRAPFRAME, KERNEL_VA2PA(trapframe) , PGSIZE, PTE_PLV0 | PTE_RPLV | PTE_MAT_CC | PTE_P | PTE_W | PTE_NX | PTE_D);

    return upgtbl;
}


// just for init proc
pagetable_t
uvminit(uint64 trapframe, const char* init_code, int sz)
{
    assert(init_code);
    assert(sz <= 16*PGSIZE);
    
    void* userspace = kalloc(sz);
    memmove(userspace, init_code, sz);
    uint64 userspace_pa = KERNEL_VA2PA(userspace);

    pagetable_t upgtbl = uvmmake(trapframe);

    mappages(upgtbl, 0, userspace_pa, sz, PTE_PLV3 | PTE_MAT_CC | PTE_P | PTE_D | PTE_W);

    // map guard page, for uvmcpoy
    mappages(upgtbl, 16*PGSIZE, 0, PGSIZE, 0);

    char* ustack = kalloc(PGSIZE);
    mappages(upgtbl, 17 * PGSIZE, (uint64)ustack, PGSIZE, PTE_PLV3 | PTE_MAT_CC | PTE_P | PTE_W | PTE_NX | PTE_D);

    return (pagetable_t) KERNEL_VA2PA(upgtbl);
}

void 
map_stack(pagetable_t pgtbl, uint64 stack_va) 
{
    pgtbl = (pagetable_t) KERNEL_PA2VA(pgtbl);
    void* stack = kalloc(KSTACK_SIZE);
    Assert(stack, "out of memory");
    // log("map stack va: %lx, pa %lx", stack_va, KERNEL_VA2PA(stack));
    mappages(pgtbl, stack_va, KERNEL_VA2PA(stack), KSTACK_SIZE, PTE_PLV0 | PTE_MAT_CC | PTE_P | PTE_NX | PTE_W | PTE_RPLV | PTE_D);
}

uint64
virt_to_phys(uint64 va) {
    return va & ~DMW_MASK;
}

uint64
phys_to_virt(uint64 pa) {
    return pa | DMW_MASK;
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
    uint64 badv = r_csr_badv();
    uint64 va = PGROUNDDOWN(badv);

    struct vm_area* vma = find_vma(p, va);
    CHECK(vma, "vma not found, badv: %lx", badv);

    int ecode = r_ecode();
    CHECK((ecode == PIS && (vma->prot & PROT_WRITE)) && (ecode == PIL && (vma->prot & PROT_READ)), "vma prot error");

    char* mem = kalloc(PGSIZE);
    CHECK(mem, "out of memory");
    memset(mem, 0, PGSIZE);

    if (vma->file) {
        // file system help
        // ilock(vma->file->ip);
        // readi(vma->file->ip, 0, (uint64)page, 
        //       vma->offset + (va - vma->start), PGSIZE);
        // iunlock(vma->file->ip);
    }

    uint perm = PTE_U | PTE_MAT_CC | PTE_P;
    perm |= ((vma->prot & PROT_READ) ? 0 : PTE_NR);
    perm |= ((vma->prot & PROT_EXEC) ? 0 : PTE_NX);
    perm |= ((vma->prot & PROT_WRITE) ? PTE_W | PTE_D : 0);

    if (vma->flags & MAP_PRIVATE) {
        perm &= ~PTE_W;
        perm |= PTE_COW;
    }
    else if (vma->flags & MAP_SHARED) {
        perm |= PTE_G;
    }

    mappages(UPGTBL(p->pagetable), va, KERNEL_VA2PA(mem), PGSIZE, perm);

    flush_tlb_one(p->pid, va);
}

void free_pgtbl(pagetable_t pgtbl, uint64 sz) {
    uvmunmap(pgtbl, TRAPFRAME, 1, UVMUNMAP_FREE);
    uvmfree(pgtbl, sz);
}
