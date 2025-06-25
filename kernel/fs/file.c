#include <fs/file.h>
#include <debug.h>
#include <proc/proc.h>
#include <mm/mm.h>
#include <fs/fcntl.h>
#include <fs/devfs/devfs.h>
#include <fs/devfs/devs/tty.h>

static struct file f_stdin = {
    .f_flags = O_RDONLY,
    .f_inode = NULL,
    .f_op = &devfs_file_fops,
    .f_path = "/dev/stdin",
    .f_private = (void*)&stdin,
    .fpos = 0,
}, f_stdout = {
    .f_flags = O_WRONLY,
    .f_inode = NULL,
    .f_op = &devfs_file_fops,
    .f_path = "/dev/stdout",
    .f_private = (void*)&stdout,
    .fpos = 0,
}, f_stderr = {
    .f_flags = O_WRONLY,
    .f_inode = NULL,
    .f_op = &devfs_file_fops,
    .f_path = "/dev/stderr",
    .f_private = (void*)&stderr,
    .fpos = 0,
};

void iofd_init() {
    atomic_init(&f_stdin.f_ref, 0);
    atomic_init(&f_stdout.f_ref, 0);
    atomic_init(&f_stderr.f_ref, 0);
}

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
    spinlock_init(&files->fdt_lock, name);

    files->fd[0] = &f_stdin;
    files->fd[1] = &f_stdout;
    files->fd[2] = &f_stderr;
    files->next_fd = 3;
    files->nr_avail_fd = NR_OPEN - 3;

    file_get(&f_stdin);
    file_get(&f_stdout);
    file_get(&f_stderr);
}

struct files_struct* fdt_dup(struct files_struct *fdt)
{
    char buffer[SPINLOCK_NAME_MAX_LEN];
    struct files_struct *new_fdt = kcalloc(sizeof(struct files_struct), 1);
    if (new_fdt == NULL) {
        error("Failed to allocate new files_struct");
        return NULL;
    }

    strncpy(buffer, fdt->fdt_lock.name, SPINLOCK_NAME_MAX_LEN - 1);
    name_append_suffix(buffer, SPINLOCK_NAME_MAX_LEN, "_dup");
    spinlock_init(&new_fdt->fdt_lock, buffer);

    spinlock_acquire(&new_fdt->fdt_lock);
    spinlock_acquire(&fdt->fdt_lock);

    for (fd_t fd = 0; fd < NR_OPEN; fd++) {
        if (fdt->fd[fd] != NULL) {
            new_fdt->fd[fd] = memdup(fdt->fd[fd], sizeof(struct file));
            new_fdt->fd[fd]->f_inode = memdup(new_fdt->fd[fd]->f_inode, sizeof(struct inode));
        }
    }

    new_fdt->next_fd = fdt->next_fd;
    new_fdt->nr_avail_fd = fdt->nr_avail_fd;

    spinlock_release(&fdt->fdt_lock);
    spinlock_release(&new_fdt->fdt_lock);

    return new_fdt;
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

    file_get(file);

    spinlock_release(&fdt->fdt_lock);

    return fd;
}

void fd_free(struct files_struct *fdt, fd_t fd) {
    if(fd < 0 || fd >= NR_OPEN)
        return;

    spinlock_acquire(&fdt->fdt_lock);

    if(fdt->fd[fd] != NULL) {
        file_put(fdt->fd[fd]);
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

void file_init(struct file *file, const struct file_operations *f_op, const char *path, unsigned int flags, void* private) {
    memset(file, 0, sizeof(struct file));
    file->f_op = f_op;
    if(path)
        strncpy(file->f_path, path, MAX_PATH_LEN - 1);
    file->f_flags = flags;
    file->fpos = 0;
    file->f_private = private;
    atomic_init(&file->f_ref, 0);
}

void file_get(struct file *file)
{
	if (file)
		atomic_inc(&file->f_ref);
}

int file_put(struct file *file)
{
	int ret = -1;
	if (file && (ret = atomic_dec(&file->f_ref)) == 0)
	{
		ret = call_interface(file->f_op, close, int, file);
		if (ret < 0) {
			error("call specified close failed");
			return -1;
		}
		if (file->f_inode)
			kfree(file->f_inode);
		kfree(file);
		return 0;
	}
	return ret;
}

int file_refcnt(struct file *file)
{
	if (file == NULL)
		return 0;

	return atomic_get(&file->f_ref);
}
