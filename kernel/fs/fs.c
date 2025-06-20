#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/stat.h>
#include <fs/dirent.h>
#include <fs/pipe.h>
#include <fs/ext4/ext4.h>
#include <fs/devfs/devfs.h>

#include <io/blk.h>
#include <proc/proc.h>
#include <syscall.h>
#include <debug.h>

#define NR_MOUNT 16

struct mountpoint mount_table[NR_MOUNT];
int mount_count = 0, i_unfixed_mp;
const struct file_system *filesys[] = {&devfs_fs, &ext4_fs};

/************************ Export and Helper functions ************************/

static const struct file_system *filesys_find(const char *fstype)
{
	int i;

	for (i = 0; i < nr_elem(filesys); i++)
		if (!strcmp(fstype, filesys[i]->name))
			return filesys[i];
	return NULL;
}

static int str_match_prefix(const char *str, const char *prefix)
{
	int p_len = strlen(prefix);
	int str_len = strlen(str);
	int end_index = 0;

	while (end_index < p_len && end_index < str_len &&
		   prefix[end_index] == str[end_index])
	{
		end_index++;
	}

	return end_index;
}

static inline int dirent_name_len(const char* str) {
	int ret = 0;
	while(*str != '\0' && *str != '/') {
		str ++;
		ret ++;
	}
	return ret;
}

static int mountpoint_find(const char *path, int start)
{
	int max_len = -1, res = -1;

	for (int i = start; i < mount_count; i++)
	{
		int len = str_match_prefix(path, mount_table[i].mountpoint) - 1;
		if (len > max_len && len + 1 == strlen(mount_table[i].mountpoint))
		{
			max_len = len;
			res = i;
		}
	}

	return res;
}

/**
 * Convert relative path to full path, remove all "." and ".."
 * use this only when path is NOT started with "/"
 * @path: path to convert
 * @full_path: buffer to store full path, should be initialized by cwd
 * @return: length of full path on success, -1 on error
 */
static int fullpath_connect(const char *path, char *full_path)
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

static int get_absolute_path(const char *path, char *full_path, fd_t dirfd)
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

int vfilesys_init() {
	devfs_init(&mount_table[mount_count++]);
	i_unfixed_mp = 1;
	iofd_init();
	return 0;
}

/************************ Syscalls for filesystems *************************/

SYSCALL_DEFINE2(getcwd, char *, char *, buf, size_t, size)
{
	const char *cwd = myproc()->cwd;

	if (buf == NULL || strlen(cwd) > size)
		return NULL;

	strncpy(buf, cwd, size);
	return buf;
}

SYSCALL_DEFINE1(dup, fd_t, fd_t, fd)
{
	struct files_struct *fdt = myproc()->fdt;
	int ret;

	ret = fd_clone(fdt, fd, -1);

	if (ret < 0)
	{
		error("fd clone error");
		return -1;
	}

	return ret;
}

SYSCALL_DEFINE3(dup3, fd_t, fd_t, old, fd_t, new, int, flags)
{
	struct files_struct *fdt = myproc()->fdt;
	int ret;

	warn_on(flags & O_CLOEXEC, "O_CLOEXEC is currently not supported.");

	ret = fd_clone(fdt, old, new);

	if (ret < 0)
	{
		error("fd clone error");
		return -1;
	}

	return ret;
}

SYSCALL_DEFINE1(chdir, int, const char *, path)
{
	char **p_cwd = &myproc()->cwd;

	kfree(*p_cwd);
	*p_cwd = strdup(path);

	if (*p_cwd == NULL)
	{
		error("strdup error");
		return -1;
	}

	return 0;
}

/**
 * openat syscall
 * We currently alloc and free BOTH inode and file in open and close
 * @dirfd: fd of dir or AT_FDCWD
 * @path: path to open
 * @flags: open flags
 * @return: fd on success, -1 on error
 */
SYSCALL_DEFINE4(openat, fd_t, fd_t, dirfd, const char *, path, int, flags, umode_t, mode)
{
	fd_t fd;
	struct mountpoint *mount_p = NULL;
	int ret, mp_index;
	char full_path[MAX_PATH_LEN];
	struct file *file = NULL;
	struct inode *inode = NULL;
	struct files_struct *fdt = myproc()->fdt;
	struct stat stat;

	debug("open path: %s, flags: %d", path, flags);

	ret = get_absolute_path(path, full_path, dirfd);
	if (ret < 0)
	{
		error("get absolute path error");
		return -1;
	}

	// find mountpoint
	mp_index = mountpoint_find(full_path, 0);
	if (mp_index < 0)
	{
		error("mountpoint not found for path %s", full_path);
		goto out_err;
	}
	mount_p = &mount_table[mp_index];
	assert(mount_p->fs != NULL);

	file = kcalloc(sizeof(struct file), 1);
	if (file == NULL)
	{
		error("alloc file error");
		goto out_err;
	}

	inode = kcalloc(sizeof(struct inode), 1);
	if (inode == NULL)
	{
		error("alloc inode error");
		goto out_file;
	}

	atomic_init(&file->f_ref, 1);
	file->f_flags = flags;
	debug("file->f_flags: %d", file->f_flags);
	ret = call_interface(mount_p->fs->fs_op, ifget, int, mount_p, inode, file);
	if (ret < 0)
	{
		error("ifget error");
		goto out_inode;
	}

	debug("open file %s, flags: %d", full_path, flags);

	ret = call_interface(file->f_op, openat, int, file, full_path, flags, mode);
	if (ret != EOK)
	{
		error("open error, ret: %d", ret);
		goto out_inode;
	}

	// add file to fdt

	fd = fd_alloc(fdt, file);
	if (fd < 0)
	{
		error("alloc fd error");
		goto out_inode;
	}

	// fill inode state info

	ret = call_interface(mount_p->fs->fs_op, getattr, int, full_path, &stat);
	if (ret != EOK)
	{
		error("stat error");
		goto out_fd;
	}

	inode->i_ino = stat.st_ino;
	inode->i_mode = stat.st_mode;
	inode->i_size = stat.st_size;
	inode->i_atime = stat.st_atime;
	inode->i_mtime = stat.st_mtime;
	inode->i_ctime = stat.st_ctime;

	strcpy(file->f_path, full_path);

	/**
	 * TODO: add inode cache machanism
	 * TODO: ensure more flags be supported
	 */

	return fd;

out_fd:
	fd_free(fdt, fd);
out_inode:
	kfree(inode);
out_file:
	kfree(file);
out_err:
	return -1;
}

SYSCALL_DEFINE3(read, ssize_t, int, fd, char *, buf, size_t, count)
{
	struct file *file;
	struct files_struct *fdt = myproc()->fdt;
	ssize_t ret;
	off_t ori_fpos;

	if (fd < 0 || fd >= NR_OPEN)
		return -1;

	file = fd_get(fdt, fd);
	if (file == NULL)
		return -1;

	if((file->f_flags & O_ACCMODE) == O_WRONLY) {
		error("file is write only");
		return -1;
	}

	ori_fpos = file->fpos;
	ret = call_interface(file->f_op, read, ssize_t, file, buf, count, &file->fpos);
	if (ret < 0)
		return -1;

	return file->fpos - ori_fpos;
}

SYSCALL_DEFINE3(write, ssize_t, int, fd, const char *, buf, size_t, count)
{
	struct file *file;
	struct files_struct *fdt = myproc()->fdt;
	ssize_t ret;
	off_t ori_fpos;

	if (fd < 0 || fd >= NR_OPEN)
		return -1;

	file = fd_get(fdt, fd);
	if (file == NULL)
		return -1;

	if((file->f_flags & O_ACCMODE) == O_RDONLY) {
		error("file is read only");
		return -1;
	}

	if(file->f_flags & O_APPEND) {
		file->fpos = call_interface(file->f_op, llseek, off_t, file, 0, SEEK_END);
	}

	ori_fpos = file->fpos;
	ret = call_interface(file->f_op, write, ssize_t, file, buf, count, &file->fpos);
	if (ret < 0)
		return -1;

	return file->fpos - ori_fpos;
}

SYSCALL_DEFINE1(close, int, int, fd)
{
	struct file *file;
	struct files_struct *fdt = myproc()->fdt;
	int ret;

	if (fd < 0 || fd >= NR_OPEN)
		return -1;

	file = fd_get(fdt, fd);
	if (file == NULL)
		return -1;

	if(atomic_dec(&file->f_ref) == 0) {
		ret = call_interface(file->f_op, close, int, file);
		if (ret < 0) {
			error("call specified close failed");
			return -1;
		}
	
		if(file->f_inode)
			kfree(file->f_inode);
		kfree(file);
	}
	else {
		debug("file ref count has not down to 1 yet");
	}

	fd_free(fdt, fd);

	return 0;
}

SYSCALL_DEFINE3(lseek, off_t, int, fd, off_t, offset, int, whence)
{
	struct file *file;
	struct files_struct *fdt = myproc()->fdt;
	int ret;

	if (fd < 0 || fd >= NR_OPEN)
		return -1;

	file = fd_get(fdt, fd);
	if (file == NULL)
		return -1;

	ret = call_interface(file->f_op, llseek, int, file, offset, whence);
	if (ret < 0)
		return -1;

	return ret;
}

SYSCALL_DEFINE2(stat, int, const char *, path, struct stat *, buf)
{
	struct mountpoint *mount_p;
	int ret, mp_index;
	char full_path[MAX_PATH_LEN];

	ret = get_absolute_path(path, full_path, AT_FDCWD);
	if (ret < 0)
	{
		error("get absolute path error");
		return -1;
	}

	// find mountpoint
	mp_index = mountpoint_find(full_path, 0);
	if (mp_index < 0)
	{
		error("mountpoint not found for path %s", full_path);
		return -1;
	}
	mount_p = &mount_table[mp_index];
	assert(mount_p->fs != NULL);

	ret = call_interface(mount_p->fs->fs_op, getattr, int, full_path, buf);
	if (ret < 0)
	{
		error("stat error");
		return -1;
	}

	return 0;
}

SYSCALL_DEFINE2(fstat, int, int, fd, struct stat *, buf)
{
	struct file *file;
	struct files_struct *fdt = myproc()->fdt;
	struct inode *inode;
	int ret;

	if (fd < 0 || fd >= NR_OPEN)
		return -1;

	file = fd_get(fdt, fd);
	if (file == NULL)
		return -1;

	inode = file->f_inode;

	ret = call_interface(inode->i_mp->fs->fs_op, getattr, int, file->f_path, buf);
	if (ret < 0)
		return -1;

	return 0;
}

SYSCALL_DEFINE3(unlinkat, int, fd_t, dirfd, const char *, path, unsigned int, flags)
{
	struct mountpoint *mount_p;
	int ret, mp_index;
	char full_path[MAX_PATH_LEN];

	ret = get_absolute_path(path, full_path, dirfd);
	if (ret < 0)
	{
		error("get absolute path error");
		return -1;
	}

	// find mountpoint
	mp_index = mountpoint_find(full_path, 0);
	if (mp_index < 0)
	{
		error("mountpoint not found for path %s", full_path);
		return -1;
	}
	mount_p = &mount_table[mp_index];
	assert(mount_p->fs != NULL);

	if (flags & AT_REMOVEDIR)
		ret = call_interface(mount_p->fs->fs_op, rmdir, int, full_path);
	else
		ret = call_interface(mount_p->fs->fs_op, unlink, int, full_path);

	if (ret < 0)
	{
		error("unlink error");
		return -1;
	}

	return 0;
}

SYSCALL_DEFINE3(getdents64, ssize_t, int, fd, struct dirent *, buf, size_t, len)
{
	struct file *file;
	struct files_struct *fdt = myproc()->fdt;
	int ret, i, posl, posr, str_len;
	size_t size;
	struct stat stat;
	struct dirent *abuf;
	const char* name;

	if (fd < 0 || fd >= NR_OPEN) return -1;

	file = fd_get(fdt, fd);
	if (file == NULL || !(file->f_flags & O_DIRECTORY))
		return -1;

	ret = call_interface(file->f_op, getdents64, int, file, buf, len);
	if(ret < 0) {
		error("fs getdents64 failed");
		return -1;
	}

	// Add other mp dents
	abuf = (struct dirent*)((char*)buf + ret);
	len -= ret;
	for(i = 0; i < mount_count; i ++) {
		if(&mount_table[i] == file->f_inode->i_mp)
			continue;
		posl = str_match_prefix(mount_table[i].mountpoint, file->f_path);
		while(mount_table[i].mountpoint[posl] == '/') posl++;
		name = mount_table[i].mountpoint + posl;
		posr = dirent_name_len(name) + posl;
		while(mount_table[i].mountpoint[posr] == '/') posr++;
		if (mount_table[i].mountpoint[posr] != '\0') continue;

		if (call_interface(mount_table[i].fs->fs_op, getattr, int, mount_table[i].mountpoint, &stat) < 0)
			continue;

		
		str_len = strlen(name);
		size = sizeof(struct dirent) + str_len + 1;

		if (len < size)
			break;

		abuf->d_ino = stat.st_ino;
		abuf->d_off = 0;
		abuf->d_type = fs_umode_to_dtype(stat.st_mode);

		memcpy(abuf->d_name, name, str_len);
		abuf->d_reclen = size;

		abuf = (struct dirent *)((uint64)abuf + size);
		len -= size;
		ret += size;
	}

	return ret;
}

SYSCALL_DEFINE3(mkdirat, int, fd_t, dirfd, const char *, path, umode_t, mode)
{
	struct mountpoint *mount_p;
	int ret, mp_index;
	char full_path[MAX_PATH_LEN];

	ret = get_absolute_path(path, full_path, dirfd);
	if (ret < 0)
	{
		error("get absolute path error");
		return -1;
	}

	// find mountpoint
	mp_index = mountpoint_find(full_path, 0);
	if (mp_index < 0)
	{
		error("mountpoint not found for path %s", full_path);
		return -1;
	}
	mount_p = &mount_table[mp_index];
	assert(mount_p->fs != NULL);

	debug("mkdir %s, mode = %d", full_path, mode);
	ret = call_interface(mount_p->fs->fs_op, mkdir, int, full_path, mode);
	if (ret < 0)
	{
		error("mkdir error");
		return -1;
	}

	return 0;
}

SYSCALL_DEFINE5(linkat, int, fd_t, olddirfd, const char *, oldpath, fd_t, newdirfd, const char *, newpath, unsigned int, flags)
{
	struct mountpoint *mount_p;
	int ret, mp_index;
	struct files_struct *fdt = myproc()->fdt;
	struct file *old_file = fd_get(fdt, olddirfd);
	char full_old_path[MAX_PATH_LEN];
	char full_new_path[MAX_PATH_LEN];

	if (flags & AT_EMPTY_PATH)
	{
		if (old_file->f_flags & O_DIRECTORY)
			return -1;

		strcpy(full_old_path, old_file->f_path);
	}
	else
	{
		ret = get_absolute_path(oldpath, full_old_path, olddirfd);
		if (ret < 0)
		{
			error("get absolute old path error");
			return -1;
		}
	}

	ret = get_absolute_path(newpath, full_new_path, newdirfd);
	if (ret < 0)
	{
		error("get absolute new path error");
		return -1;
	}

	// find mountpoint
	mp_index = mountpoint_find(full_old_path, 0);
	if (mp_index < 0)
	{
		error("mountpoint not found for path %s", full_old_path);
		return -1;
	}
	mount_p = &mount_table[mp_index];
	assert(mount_p->fs != NULL);

	ret = call_interface(mount_p->fs->fs_op, link, int, full_old_path, full_new_path);
	if (ret < 0)
	{
		error("link error");
		return -1;
	}

	if (flags & AT_SYMLINK_FOLLOW && S_ISLNK(old_file->f_inode->i_mode))
	{
		ret = call_interface(mount_p->fs->fs_op, unlink, int, old_file->f_path);

		if (ret < 0)
		{
			error("unlink error");
			return -1;
		}
	}

	return 0;
}

SYSCALL_DEFINE3(symlinkat, int, const char *, target, fd_t, newdirfd, const char *, linkpath)
{
	struct mountpoint *mount_p;
	int ret, mp_index;
	char full_linkpath[MAX_PATH_LEN];

	ret = get_absolute_path(linkpath, full_linkpath, newdirfd);
	if (ret < 0)
	{
		error("get absolute link path error");
		return -1;
	}

	// find mountpoint
	mp_index = mountpoint_find(target, 0);
	if (mp_index < 0)
	{
		error("mountpoint not found for path %s", target);
		return -1;
	}
	mount_p = &mount_table[mp_index];
	assert(mount_p->fs != NULL);

	ret = call_interface(mount_p->fs->fs_op, symlink, int, target, full_linkpath);
	if (ret < 0)
	{
		error("symlink error");
		return -1;
	}

	return 0;
}

/**
 * mount syscall
 * We currently ignore flags
 */
SYSCALL_DEFINE5(mount, int, const char *, special, const char *, dir, const char *, fstype, unsigned long, flags, const void *, data)
{
	int len, ret, mp_index;
	char full_path_sp[MAX_PATH_LEN], full_path_dir[MAX_PATH_LEN];
	struct mountpoint *mp;
	struct blkdev *blkdev;

	ret = get_absolute_path(special, full_path_sp, AT_FDCWD);
	if (ret < 0)
	{
		error("get absolute special path error");
		return -1;
	}

	len = str_match_prefix(full_path_sp, "/dev/");

	if (len != 5)
	{
		error("cannot find special device.");
		return -1;
	}

	struct devfs_device* device = devfs_get_by_path(full_path_sp);
	if (device == NULL || device->file_type != FT_BLKDEV)
	{
		error("block device %s not found", special + len);
		return -1;
	}
	blkdev = device->disk.blkdev;

	ret = get_absolute_path(dir, full_path_dir, AT_FDCWD);
	if (ret < 0)
	{
		error("get absolute dir path error");
		return -1;
	}

	mp_index = mountpoint_find(full_path_dir, i_unfixed_mp);
	if (mp_index != -1)
	{
		error("Mount point already used.");
		return -1;
	}

	mp_index = mount_count++;
	mp = &mount_table[mp_index];

	assert(mp->fs == NULL);

	mp->blkdev = blkdev;
	mp->device = device;
	mp->fs = filesys_find(fstype);
	mp->mountpoint = strdup(full_path_dir);

	return call_interface(mp->fs->fs_op, mount, int, blkdev, mp, data);
}

SYSCALL_DEFINE2(umount2, int, const char *, special, int, flags)
{
	int ret, mp_index;
	char full_path_sp[MAX_PATH_LEN];
	struct mountpoint *mp;

	ret = get_absolute_path(special, full_path_sp, AT_FDCWD);
	if (ret < 0)
	{
		error("get absolute special path error");
		return -1;
	}

	mp_index = mountpoint_find(full_path_sp, i_unfixed_mp);
	if (mp_index < 0)
	{
		error("Mount point not found.");
		return -1;
	}

	mp = &mount_table[mp_index];

	assert(mp != NULL);
	assert(mp->fs != NULL);
	assert(mp->fs->fs_op != NULL);

	ret = call_interface(mp->fs->fs_op, umount, int, mp);
	if (ret < 0)
	{
		error("Umount mount point failed.");
		return -1;
	}

	return 0;
}

/**
 * Create a pipe
 * @param pipefd[0] refers to the read end of the pipe.
 * @param pipefd[1] refers to the write end of the pipe
 * @param flags flag
 */
SYSCALL_DEFINE2(pipe2, int, int*, pipefd, int, flags) {
	int ret;
	struct files_struct *fdt = myproc()->fdt;
	KCALLOC(struct file, rfile, 1);
	KCALLOC(struct file, wfile, 1);

	warn_on(flags, "pipe2 currently do not support flags");

	ret = pipe_init(rfile, wfile);
	if(ret < 0) {
		error("pipe init error");
		goto out_file;
	}

	atomic_init(&rfile->f_ref, 1);
	rfile->f_flags = O_RDONLY;
	atomic_init(&wfile->f_ref, 1);
	wfile->f_flags = O_WRONLY;

	pipefd[0] = fd_alloc(fdt, rfile);
	if(pipefd[0] < 0) {
		ret = -1;
		error("read fd alloc error");
		goto out_pipe;
	}

	pipefd[1] = fd_alloc(fdt, wfile);
	if(pipefd[1] < 0) {
		ret = -1;
		error("write fd alloc error");
		goto out_rfd;
	}

	return 0;

out_rfd:
	fd_free(fdt, pipefd[0]);
out_pipe:
	pipe_free((struct pipe*)wfile->f_private);
	pipe_free((struct pipe*)rfile->f_private);
out_file:
	kfree(wfile);
	kfree(rfile);
	return ret;
}