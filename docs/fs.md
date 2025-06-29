# 文件系统兼容层

HANAOS 实现了轻量级的，基于路径字符串的文件系统兼容层，该兼容层提供了文件系统的注册接口以及文件相关操作的系统调用。下面从几个方面介绍。

## 文件相关

### struct inode

```c
struct inode
{
    umode_t i_mode; // file mode
    off_t i_size;   // file size

    uint32 i_ino;   // inode number

    spinlock_t i_lock;   // inode lock
    uint32 i_refcount;   // reference count

    time_t i_atime; // last access time
    time_t i_mtime; // last modification time
    time_t i_ctime; // last status change time

    struct mountpoint *i_mp; // mountpoint
};
```

inode 保存了文件节点的基本原数据信息。目前它只有保存数据的功能，未来他会成为文件的缓存和操作结构。

### struct file

```c
struct file
{
	unsigned int f_flags;
	const struct file_operations *f_op;
	struct inode *f_inode;

	char f_path[MAX_PATH_LEN];

	off_t fpos;

	void *f_private;

	atomic_define(int) f_ref;
};
```

`struct file`是内核中对打开的文件的结构，记录了此次打开文件的 flag、path 等，并关联了 inode 文件元信息。文件结构有一个引用计数字段，它下降到 0 将会导致该 `struct file` 的释放。

对每个 file，根据挂载的文件系统不同有着注册的不同的文件操作接口，定义如下：

```c
struct file_operations
{
	off_t (*llseek)(struct file *, off_t, int);
	ssize_t (*read)(struct file *, char *, size_t, off_t *);
	ssize_t (*write)(struct file *, const char *, size_t, off_t *);
	int (*openat)(struct file *, path_t, int, umode_t);
	int (*close)(struct file *);
	int (*getdents64)(struct file *, struct dirent *, size_t);
	int (*truncate)(struct file*, off_t length);
};
```

每个接口对应了一个同名的系统调用，表示该系统调用要对文件进行的操作。

每个进程中保存了一个`struct file`数组结构`struct files_struct`，称为 fdt 表，其下标称为 fd (file descriptor)，定义如下：

```c
struct files_struct
{
	struct file *fd[NR_OPEN];
	unsigned int next_fd; // we maintain a next_fd after any fd inc/dec
	unsigned int nr_avail_fd;
	spinlock_t fdt_lock; // lock for fd table
};
```

每个进程初始化的时候，会对它的 fdt 进行初始化，并在数组的前三个元素中填入 stdin, stdout, stderr 三个特殊的文件结构，表示屏幕的标准 I/O 流。对于新打开的文件可以用`fd_alloc`将其插入 fdt 并分配 fd，除此之外有对 fd 和 fdt 的复制、删除等函数。

## 文件系统和挂载点相关

### struct file_system

```c
struct file_system
{
    const char *name;
    const struct fs_operations *fs_op;
};
```

`struct file_system`是对文件系统的抽象，包含了文件系统的注册接口：

```c
struct fs_operations
{
    int (*mount)(struct blkdev *, struct mountpoint *, const char *);
    int (*umount)(struct mountpoint *);
    int (*ifget)(struct mountpoint *, struct inode *, struct file *);
    int (*link)(path_t, path_t);
    int (*unlink)(path_t);
    int (*symlink)(path_t, path_t);
    int (*mkdir)(path_t, umode_t);
    int (*rmdir)(path_t);
    int (*rename)(path_t, path_t);
    int (*getattr)(path_t, struct stat *);
};
```

注册接口大多是对同名系统调用的相同功能，有两个特殊的接口：ifget 用于初始化填充对应文件系统的 `struct file` 和 `struct inode`，`getattr`是`stat`类系统调用对应的接口。

### struct mountpoint

```c
struct mountpoint
{
    const char *mountpoint;
    const struct file_system *fs;
    const struct blkdev *blkdev;
    const struct devfs_device *device;
    struct list_head mp_entry;
    void* private;
};
```

`struct mountpoint`表示一个挂载点，包含了挂载的文件系统、挂载的设备等信息，每个实例会被加入到一个全局的挂载点链表中。`mount`和`umount`系统调用会直接对挂载点链表进行操作，除此之外几乎所有传入路径的文件相关的系统调用都需要调用`mountpoint_find`接口，找到完整路径对应的挂载点，再调用挂载点中对应的文件系统接口。

## devfs

devfs 是一个特殊的文件系统，他只在运行时装载和保存数据并且是只读的，挂在在`/dev`目录下，用于管理所有用户可见的 I/O 设备，包括块设备和字符设备。但 devfs 管理的两种设备并不是设备抽象层的结构，而是进行了包装的 disk 和 tty 设备。

### struct devfs_device

```c
struct devfs_device {
    unsigned int file_type;
    
    union {
        struct disk disk;
        struct tty tty;
        uint8 reserved;
    };

    const char* name;

    uint32 ino;
    struct list_head dev_entry;
};
```

`struct devfs_device`是对 devfs 中注册的一个设备的抽象，目前包含了串口设备 tty，磁盘设备 disk 两种。和设备抽象层类似，所有注册的`struct devfs_device`都会被保存在链表中，在系统试图列出`/dev`目录的目录项时，这些设备名称会作为目录项名称被返回。

devfs 在初始化时，会对两种设备进行添加。首先，获取默认的字符设备作为底层设备，在他的基础上注册三个串口设备 stdin, stdout, stderr 并设置各自的缓存方式（见下文对 tty 的叙述），加入到链表中；然后，遍历设备抽象层所有的块设备，按照`sda, sdb, ..., sdz`的名称顺序注册为磁盘设备。

devfs 分别注册了一套`struct file_operations devfs_file_fops`和`struct fs_operations devfs_filesystem_ops`，对其中接口的调用将会自动识别设备的类型并转到对应的设备操作。这意味着用户可以直接对磁盘进行 I/O 操作，绕过文件系统。

### struct tty

```c
struct tty {
    struct chrdev* chrdev;
    char name[MAX_FILENAME_LEN];
    char *buffer; // NULL for no buffer
    size_t bofs, bufsize;
};
```

tty 设备关联了一个字符设备，并设置了可能的缓冲区指针，在 tty 设备初始化时可以设置有无缓冲。例如，stdout 采用了行缓冲，而 stdin 和 stderr 无缓冲。

### struct disk

```c
struct disk {
    struct blkdev* blkdev;
    char name[MAX_FILENAME_LEN];
    char *buffer; // io buffer in sector size, used for last sector
    off_t ofs;
};
```

struct disk 将一个磁盘模拟为一个文件。为了在最后一个扇区也提交完整的扇区进行 I/O，也包含了一个缓冲区。挂载系统调用的挂载设备就是指向了一个`struct disk`。注意，`struct disk`可以重写 I/O 函数，这意味着可以对磁盘进行分区读写。

## 管道

```c
struct pipe {
	struct kfifo* kfifo;
	spinlock_t* lock;
	int ref;
};
```

HANAOS 使用了环形队列数据结构`struct kfifo`管理缓冲区，实现了管道，用于进程之间的通信。

管道也实现了自己的一套`struct file_operations pipe_fileops`，这会在访问对应 fd 时调用对应的管道文件接口。