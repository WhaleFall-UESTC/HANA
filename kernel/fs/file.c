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
        }
    }
    fdt->next_fd = NR_OPEN;
}

void fdt_init(struct files_struct *files)
{
    memset(files, 0, sizeof(*files));
    files->next_fd = 0;
    files->nr_avail_fd = NR_OPEN;
}

fd_t fd_alloc(struct files_struct *fdt, struct file* file)
{
    fd_t fd = fdt->next_fd;
    
    if(file == NULL)
    {
        error("file is NULL");
        return -1;
    }

    if (fd >= NR_OPEN)
    {
        error("too much open files for porc %s", myproc()->name);
        return -1;
    }

    assert(fdt->fd[fd] == NULL);

    fdt->fd[fd] = file;
    find_avail_fd(fdt);

    fdt->nr_avail_fd--;
    return fd;
}

void fd_free(struct files_struct *fdt, fd_t fd) {
    if(fd < 0 || fd >= NR_OPEN)
        return;

    if(fdt->fd[fd] != NULL) {
        fdt->fd[fd] = NULL;
        fdt->nr_avail_fd++;
        find_avail_fd(fdt);
    }
}