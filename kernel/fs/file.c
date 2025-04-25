#include <fs/file.h>
#include <debug.h>
#include <proc/proc.h>
#include <mm/mm.h>

static inline void find_avail_fd(struct files_struct *fdt)
{
    if(!fdt->nr_avail_fd) {
        fdt->next_fd = NR_OPEN;
        return;
    }

    for(fd_t fd = 0; fd < NR_OPEN; fd++)
    {
        if (fdt->fd[fd] == NULL)
        {
            fdt->next_fd = fd;
            return;
        }
    }
    fdt->next_fd = NR_OPEN;
}

void fdt_init(struct files_struct *files, char* name)
{
    debug("fdt addr: 0x%lx", (uint64)files);
    debug("sizeof fdt: %lu", sizeof(struct files_struct));
    memset(files, 0, sizeof(struct files_struct));
    files->next_fd = 0;
    files->nr_avail_fd = NR_OPEN;
    spinlock_init(&files->fdt_lock, name);
}

fd_t fd_alloc(struct files_struct *fdt, struct file* file)
{
    fd_t fd;
    
    if(file == NULL)
    {
        error("file is NULL");
        return -1;
    }

    spinlock_acquire(&fdt->fdt_lock);

    if ((fd = fdt->next_fd) >= NR_OPEN)
    {
        error("too much open files for porc %s", myproc()->name);
        return -1;
    }

    assert(fdt->fd[fd] == NULL);

    fdt->fd[fd] = file;
    find_avail_fd(fdt);

    fdt->nr_avail_fd--;

    spinlock_release(&fdt->fdt_lock);

    return fd;
}

void fd_free(struct files_struct *fdt, fd_t fd) {
    if(fd < 0 || fd >= NR_OPEN)
        return;

    spinlock_acquire(&fdt->fdt_lock);

    if(fdt->fd[fd] != NULL) {
        fdt->fd[fd] = NULL;
        fdt->nr_avail_fd++;
        find_avail_fd(fdt);
    }

    spinlock_release(&fdt->fdt_lock);
}

int fd_clone(struct files_struct *fdt, fd_t old_fd, fd_t new_fd) {
    spinlock_acquire(&fdt->fdt_lock);

    if(new_fd == -1)
        new_fd = fdt->next_fd;

    if(fdt->fd[old_fd] == NULL || fdt->fd[new_fd] != NULL)
        return -1;
    
    fdt->fd[new_fd] = fdt->fd[old_fd];

    find_avail_fd(fdt);

    spinlock_release(&fdt->fdt_lock);

    return new_fd;
}

struct file* fd_get(struct files_struct *fdt, fd_t fd) {
    struct file* res;

    spinlock_acquire(&fdt->fdt_lock);

    res = fdt->fd[fd];

    spinlock_release(&fdt->fdt_lock);

    return res;
}