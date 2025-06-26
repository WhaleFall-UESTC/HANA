#include <common.h>
#include <klib.h>
#include <debug.h>
#include <arch.h>
#include <irq/interrupt.h>
#include <trap/context.h>
#include <proc/proc.h>
#include <trap/trap.h>
#include <proc/sched.h>
#include <mm/mm.h>
#include <mm/memlayout.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/kernel.h>
#include <syscall.h>
#include <elf.h>

SYSCALL_DEFINE5(clone, int, unsigned long, flags, void*, stack, void*, ptid, void*, tls, void*, ctid)
{
    struct proc* proc  = myproc();
    struct proc* child = alloc_proc();
    assert(child);

    // prepare userspace vm
    if (flags & CLONE_VM) {
        // share space with parent
        child->pagetable = upgtbl_clone(proc->pagetable);
        // TODO: mmap area copy
        for (struct vm_area *vma = proc->vma_list; vma; vma = vma->next) {
            KALLOC(struct vm_area, new_vma);
            *new_vma = *vma;
            new_vma->next = child->vma_list;
            if (child->vma_list) child->vma_list->prev = new_vma;
            child->vma_list = new_vma;
        }
        child->mmap_base = proc->mmap_base;
        child->mmap_brk = proc->mmap_brk;
    } else {
        // initialize child pagetable
        pagetable_t cpgtbl = alloc_pagetable();
        // Copy memory from parent (COW)
        uvmcopy(cpgtbl, UPGTBL(proc->pagetable), proc->sz);
    }

    child->sz = proc->sz;
    child->heap_start = proc->heap_start;

    *(child->trapframe) = *(proc->trapframe);
    // set tls
    child->tls = (uint64) tls;
    child->trapframe->tp = (uint64) tls;
    // set child process return 0
    child->trapframe->a0 = 0;

    if (flags & CLONE_FS) {
        // share fs with parent
        child->cwd = proc->cwd;
        // child->root = proc->root;
        // child->fs = proc->fs;
        // child->umask = proc->umask;
    } else {
        // initialize child fs
        child->cwd = strdup(proc->cwd);
        // child->root = strdup(proc->root);
        // child->fs = proc->fs; // share fs
        // child->umask = proc->umask;
    }

    if (flags & CLONE_FILES) {
        child->fdt = proc->fdt; // share file descriptor table
    }
    else {
        child->fdt = fdt_dup(proc->fdt);
    }

    // if (flags & CLONE_THREAD) {
    //     child->tgid = proc->tgid;
    // }

    if (flags & CLONE_PARENT_SETTID){
        copyout(UPGTBL(proc->pagetable), (uint64) ptid, &child->pid, sizeof(child->pid));
    }

    if (flags & CLONE_CHILD_SETTID) {
        copyout(UPGTBL(proc->pagetable), (uint64) ctid, &child->pid, sizeof(child->pid));
    }

    if (flags & CLONE_CHILD_CLEARTID) {
        child->cleartid = 1;
    }

    if (flags & CLONE_SIGCHLD) {
        child->sigchld = 1;
    }

    child->parent = proc;
    child->state = RUNNABLE;

    // add child to proc_list
    child->next = proc_list;
    proc_list->prev = child;
    proc_list = child;

    return child->pid;
}

#define LOADER_CHECK(cond) \
    if (!(cond)) goto bad

SYSCALL_DEFINE3(execve, int, const char*, upath, const char**, uargv, const char**, uenvp)
{
    if (upath == NULL)
        return -1;

    struct proc* p = myproc();
    pagetable_t pgtbl = NULL;
    pagetable_t old_pgtbl = UPGTBL(p->pagetable);
    struct file* file;
    // sz is a pointer, point at the current top of virtual user space
    uint64 sz = 0;

    char path[PATH_MAX];
    if (copyinstr(old_pgtbl, path, (uint64) upath, PATH_MAX) < 0) {
        error("Path too long");
        return -1;
    }
    
    Elf64_Ehdr elf = {};

    file = kernel_open(path);
    if(file == NULL) {
        error("open path failed");
        return -1;
    }

    kernel_read(file, &elf, sizeof(Elf64_Ehdr));
    LOADER_CHECK(*(uint*)(&elf.e_ident) == ELF_MAGIC);
    
    Elf64_Phdr phdr = {};
    pgtbl = uvmmake((uint64) p->trapframe);

    for (int i = 0, off = elf.e_phoff; i < elf.e_phnum; i++, off += sizeof(Elf64_Phdr))
    {
        kernel_lseek(file, off, SEEK_SET);
        kernel_read(file, &phdr, sizeof(Elf64_Ehdr));

        if (phdr.p_type != PT_LOAD)
            continue;

        LOADER_CHECK(phdr.p_memsz >= phdr.p_filesz);
        LOADER_CHECK(phdr.p_vaddr + phdr.p_memsz >= phdr.p_vaddr);
        
        // load to memory
        LOADER_CHECK(IS_PGALIGNED(phdr.p_vaddr));

        int perms = PTE_U;
        if (phdr.p_flags & (PF_R | PF_W)) perms |= PTE_RW;
        else if (phdr.p_flags & (PF_R | PF_X)) perms |= PTE_RX;
        else if (phdr.p_flags & (PF_R)) perms |= PTE_RONLY;
        else perms |= PTE_RWX;
        
        char* mem = kalloc(phdr.p_memsz);
        mappages(pgtbl, phdr.p_vaddr, KERNEL_VA2PA(mem), phdr.p_memsz, perms);

        sz = max_uint64(sz, phdr.p_vaddr + phdr.p_memsz);

        kernel_lseek(file, phdr.p_offset, SEEK_SET);
        kernel_read(file, mem, phdr.p_filesz);
    }

    kernel_close(file);

    // now map user stack 
    // first we leave a protect page, which is blank and unmapped
    sz = PGROUNDUP(sz) + PGSIZE;
    // next, alloc and map user stack
    char* ustack = kalloc(PGSIZE);
    mappages(pgtbl, sz, KERNEL_VA2PA(ustack), PGSIZE, PTE_U | PTE_RW);
    pte_t* pte = walk(pgtbl, sz, WALK_NOALLOC);

    sz += PGSIZE;
    // stack pointer, now point at top
    uint64 sp = sz, stack_top = sz;
    // user stack physical page
    char* ustack_p = (char*) KERNEL_PA2VA(PTE2PA(*pte));

    // prepare argv, envp in user stack
    // first, copy them from user space to kernel
    const int max_size = MAX_ARGS * sizeof(char*);
    char* argv[max_size];
    char* envp[max_size];
    copyin(old_pgtbl, (void*)argv, (uint64)uargv, max_size);
    copyin(old_pgtbl, (void*)envp, (uint64)uenvp, max_size);

    int argc, envc;
    for (argc = 0; argv[argc]; argc++);
    for (envc = 0; envp[envc]; envc++);

    // copy all envp string to a tempoary memory
    // caculate the real space they need
    // then copy to ustack
    char* envp_mem = kalloc(envc * MAX_ARG_STRLEN);
    uint64 envp_nbytes = 0;
    for (int i = 0; i < envc; i++) {
        int size = copyinstr(old_pgtbl, &envp_mem[envp_nbytes], (uint64) envp[i], MAX_ARG_STRLEN);
        if (size < 0) {
            kfree(envp_mem);
            goto bad;
        }
        size = ALIGN(size, 8);
        // store offsets of each string
        envp[i] = (char*) envp_nbytes;
        envp_nbytes += size;
    }

    // copy envp string to stack
    memmove(&ustack_p[PGSIZE - envp_nbytes], envp_mem, envp_nbytes);
    kfree(envp_mem);

    sp -= envp_nbytes;
    uint64 p_envp = sp - 2 * 8;

    // caculate envp string address, and store it
    int addr = PGSIZE - envp_nbytes - 8;
    for (int i = 0; i < envc; i++) {
        addr -= 8;
        *((uint64*)(&ustack_p[addr])) = ((uint64) envp[i]) + sp;
    }

    sp -= 8;
    sp -= envc * 8;
    sp -= 8;

    // copy all arg string to a tempoary memory
    // caculate the real space they need
    // then copy to ustack
    char* argv_mem = kalloc(argc * MAX_ARG_STRLEN);
    uint64 argv_nbytes = 0;
    for (int i = 0; i < argc; i++) {
        int size = copyinstr(old_pgtbl, &argv_mem[argv_nbytes], (uint64) argv[i], MAX_ARG_STRLEN);
        if (size < 0) {
            kfree(argv_mem);
            goto bad;
        }
        size = ALIGN(size, 8);
        // store offsets of each string
        argv[i] = (char*) argv_nbytes;
        argv_nbytes += size;
    }

    // copy envp string to stack
    memmove(&ustack_p[PGSIZE - (stack_top - sp) - argv_nbytes], argv_mem, argv_nbytes);
    kfree(argv_mem);

    sp -= argv_nbytes;
    uint64 p_argv = sp - 2 * 8;

    // caculate envp string address, and store it
    int addr_ = PGSIZE - (stack_top - sp) - 8;
    for (int i = 0; i < argc; i++) {
        addr_ -= 8;
        *((uint64*)(&ustack_p[addr_])) = ((uint64) argv[i]) + sp;
    }
    
    sp -= 8;
    sp -= argc * 8;
    sp -= 8;

    // store argc
    sp -= 8;
    *((uint64*)(&ustack_p[PGSIZE - (stack_top - sp)])) = (uint64) argc;

    p->trapframe->a1 = p_argv;
    p->trapframe->a2 = p_envp;

    // free old pagetable
    proc_free_pagetable(p);
    p->pagetable = upgtbl_init(pgtbl);
    p->sz = sz;
    p->heap_start = sz;
    trapframe_set_era(p, elf.e_entry);
    p->trapframe->sp = sp;

    return argc;

bad:
    if (pgtbl) {
        uvmfree(pgtbl, sz);
        freewalk(pgtbl, 2);
    }
    
    return -1;
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
// Return 0 if not found and set WNOHANG
// WUNTRACED，WCONTINUED are not implemented
SYSCALL_DEFINE3(wait4, int, int, pid, int*, status, int, options)
{
    struct proc* curproc = myproc();

    for (;;) {
        int found = 0;
        int has_child = 0;
        
        for (struct proc* p = proc_list; p; p = p->next) {
            if (p->parent == curproc && !p->waited) {
                has_child = 1;

                if (p->state == ZOMBIE) {
                    found = 1;

                    if (status != NULL) {
                        int child_status = p->status;
                        if (copyout(UPGTBL(p->pagetable), (uint64)status, &child_status, sizeof(int)) < 0) {
                            return -1;
                        }
                    }
                    
                    // its parent is waiting for it
                    p->waited = 1;
                    int pid = p->pid;

                    freeproc(p);

                    return pid;
                }
            }
        }

        if (!found) {
            if (!has_child || curproc->killed)
                return -1;

            if (options & WNOHANG)
                return 0;

            sleep(curproc);
        }
    }
}


SYSCALL_DEFINE1(exit, int, int, ec) {
    do_exit(ec);
    return 0;
}

SYSCALL_DEFINE0(getppid, int) {
    return myproc()->parent->pid;
}

SYSCALL_DEFINE0(getpid, int) {
    return myproc()->pid;
}

SYSCALL_DEFINE0(sched_yield, int) {
    yield();
    return 0;
}
