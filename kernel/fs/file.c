#include <fs/file.h>
#include <debug.h>
#include <proc/proc.h>
#include <mm/mm.h>

static inline void find_avail_fd(struct files_struct *fdt)
{
    for(fd_t fd = 0; fd < NR_OPEN; fd++)
    {
        if (fdt->fd[fd] == NULL)
        {
            fdt->next_fd = fd;
            return fd;
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

fd_t fd_alloc(struct files_struct *fdt)
{
    fd_t fd = fdt->next_fd;
    if (fd >= NR_OPEN)
    {
        error("too much open files for porc %s", myproc()->name);
        return -1;
    }
    fdt->fd[fd] = kcalloc(1, sizeof(struct file));
    if (fdt->fd[fd] == NULL)
    {
        error("alloc file error");
        return -1;
    }

    fdt->nr_avail_fd--;
    return fd;
}