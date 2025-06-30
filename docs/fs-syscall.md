# 文件系统相关系统调用

HANAOS 在文件系统兼容层实现了丰富的文件操作和文件系统操作接口，这意味着文件系统相关的系统调用只需对传入参数进行适当处理之后调用相关注册接口即可。下面介绍系统调用主要的参数处理流程和接口调用方式。

## 路径处理

当系统调用传入一个文件路径时，首先要做的是将其拷贝到内核空间：

```c
if(copy_from_user_str(__path, path, MAX_PATH_LEN) < 0) {
    error("copy from userspace error");
    return -1;
}
```

一般来说，传入的路径都是相对路径，需要和当前工作路径 cwd 拼接之后获得绝对路径，再传递给下层接口使用。HANAOS 实现了几个 helper 函数，能方便的将相对路径转换为绝对路径：

```c
int fullpath_connect(const char *path, char *full_path)
{
	int path_len = strlen(path);
	int i_path = 0, i_f = strlen(full_path);

	if (path[0] == '/')
		return -1;

	if (full_path[i_f - 1] != '/')
		full_path[i_f ++] = '/';

	while (i_path < path_len && i_f < MAX_PATH_LEN)
	{
		while (path[i_path] == '/' && i_path < path_len)
			i_path++;
		if (path[i_path] == '.' && (i_path + 2 == path_len || (i_path + 2 < path_len && path[i_path + 2] == '/')) && path[i_path + 1] == '.')
		{
			// "../" or "..\0"
			i_path += 2;
			i_f --;
			while (i_f > 0 && full_path[i_f - 1] != '/')
				i_f--;
			if (i_f > 0)
				i_f--;
		}
		else if (path[i_path] == '.' && (i_path + 1 == path_len || (i_path + 1 < path_len && path[i_path + 1] == '/')))
		{
			// "./" or ".\0"
			i_path++;
		}
		else
		{
			// normal path
			while (path[i_path] != '/' && i_path < path_len && i_f < MAX_PATH_LEN)
			{
				full_path[i_f++] = path[i_path++];
			}
		}
	}

	if (i_path < path_len)
	{
		error("path too long");
		return -1;
	}

	full_path[i_f] = '\0';
	return i_f;
}

int get_absolute_path(const char *path, char *full_path, fd_t dirfd)
{
	struct files_struct *fdt = myproc()->fdt;
	struct file *file;

	if (path == NULL || full_path == NULL)
	{
		error("path or full_path is NULL");
		return -1;
	}

	if (path[0] == '/')
	{
		strcpy(full_path, path);
	}
	else
	{
		// convert relative path to full path
		if (dirfd == AT_FDCWD)
			strcpy(full_path, myproc()->cwd);
		else
		{
			file = fd_get(fdt, dirfd);
			strcpy(full_path, file->f_path);
		}
		int ret = fullpath_connect(path, full_path);
		if (ret < 0)
			return -1;
	}

	return 0;
}
```

其中`fullpath_connect`函数获得相对路径和存放绝对路径的缓冲区，其中后者已经拷贝了相对路径的先导（即要拼合的路径）。

`get_absolute_path`将相对路径拼合到给定的 fd 代表的路径之后，这个函数的行为和以 at 结尾的文件系统系统调用的行为一致。当传入的 fd 是`AT_FDCWD`时，从当前工作路径开始拼合；否则从 fd 描述的路径开始。该函数按照 fd 拷贝先导路径之后，调用`fullpath_connect`函数。

这种路径处理机制相比 Linux 的直接对 inode 和 file 的缓存进行处理的方式来说比较低效，但是具有很强的兼容性，更加方便了 lwext4 等嵌入式文件系统的移植。

获取到完整路径之后，通常会调用`mountpoint_find`函数找到挂载点，此后就可以通过挂载点对应的文件系统调用相关操作了。

## fd 操作

当系统调用要打开新的文件并获取 fd 或者对 fd 进行复制、关闭等，file 的管理机制提供了一系列函数进行这些操作：

1. `sys_openat`中，调用相关接口获取`struct file`之后，通过`fd = fd_alloc(fdt, file)`函数将这个 file 结构体插入到进程的 fdt 表中并获得 fd；
2. `dup`相关系统调用中，使用`fd_clone`函数进行 fd 的复制；
3. 需要对 fd 进行操作的系统调用，通过`fd_get`获取保存的`struct file`；
4. `sys_close`中，使用`fd_free`来释放给定的 fd，此函数会将相应的`struct file`从 fdt 中分离并将其引用计数减一。如果这个文件结构已经没有进程引用了，他就会被释放。

## 接口调用

HANAOS 使用`call_interface`宏对接口是否注册进行检查，并调用：

```c
#define call_interface(regipt, interface, rettype, ...)                   \
	({                                                                    \
		rettype __ret;                                                    \
		if ((regipt)->interface == NULL)                                  \
		{                                                                 \
			error(macro_param_to_str(interface)                           \
				  "not supported in current fs/file.");                   \
			__ret = -1;                                                   \
		}                                                                 \
		else                                                              \
		{                                                                 \
			__ret = (regipt)->interface(__VA_ARGS__);                     \
		}                                                                 \
		__ret;                                                            \
	})
```

例如，`sys_openat`中对`openat`接口进行调用：

```c
ret = call_interface(file->f_op, openat, int, file, full_path, flags, mode);
```

文件系统相关的接口调用主要为调用 file operations 和 filesystem opertions，这取决于系统调用操作的对象。值得注意的是，`sys_openat`会先调用文件系统的`ifget`接口获取文件结构和元信息，然后才会调用文件操作的`open`接口，这意味着一个系统调用可能会调用不同类别的多个接口。

文件系统的系统调用具体实现均遵守以上原则，没有更多额外的机制，此处不再赘述。