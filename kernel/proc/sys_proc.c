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
#include <syscall.h>


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
