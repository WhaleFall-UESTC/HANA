#ifndef __FS_H__
#define __FS_H__

#include <common.h>
#include <locking/spinlock.h>
#include <io/blk.h>
#include <fs/stat.h>
#include <fs/fcntl.h>

// struct path
// {
//     int p_len;
//     char p_name[0];
// };

#define MAX_PATH_LEN 1024
#define MAX_FILENAME_LEN 256
typedef const char *path_t;

struct inode_operations;
struct mountpoint;

struct inode
{
    umode_t i_mode; // file mode
    off_t i_size;   // file size

    // const struct inode_operations *i_op; // inode operations

    uint32 i_ino;              // inode number

    spinlock_t i_lock;   // inode lock
    uint32 i_refcount; // reference count

    time_t i_atime; // last access time
    time_t i_mtime; // last modification time
    time_t i_ctime; // last status change time

    struct mountpoint *i_mp; // mountpoint
};

// struct dentry;

/**
 * TODO: Let the interfaces accept inode and dentry as a parameter rather than path
 * This may lead to a lot of changes in lwext4
 */

// struct inode_operations
// {
// int (*readlink) (struct dentry *, char __user *,int);
// int (*rename)(struct inode *, path_t,
//               struct inode *, path_t, unsigned int);
// int (*setattr)(path_t, struct iattr *);
// ssize_t (*listxattr)(path_t, char *, size_t);
// };

// struct inode_operations {
// int (*permission) (struct inode *, int);

// 	int (*readlink) (struct dentry *, char __user *,int);

// 	int (*link) (struct dentry *,struct inode *,struct dentry *);
// 	int (*unlink) (struct inode *,struct dentry *);
// 	int (*symlink) (struct inode *,struct dentry *,const char *);
// 	int (*mkdir) (struct inode *,struct dentry *,umode_t);
// 	int (*rmdir) (struct inode *,struct dentry *);
// int (*mknod) (struct inode *,struct dentry *,umode_t,devid_t);
// 	int (*rename) (struct inode *, struct dentry *,
// 			struct inode *, struct dentry *, unsigned int);
//     int (*setattr) (struct dentry *, struct iattr *);
//     int (*getattr) (const path_t, struct kstat *, uint32, unsigned int);
//     ssize_t (*listxattr) (struct dentry *, char *, size_t);
// };

struct file;
struct mountpoint;

struct fs_operations
{
    int (*mount)(struct blkdev *, struct mountpoint *, const char *);
    int (*umount)(struct mountpoint *);
    // int (*statfs)(const char *mount_point, struct statfs *);
    /**
     * get fs specific inode and file
     * it init ops ptr and private data, but DO NOT alloc inode and file
     */
    int (*ifget)(struct mountpoint *, struct inode *, struct file *);
    int (*link)(path_t, path_t);
    int (*unlink)(path_t);
    int (*symlink)(path_t, path_t);
    int (*mkdir)(path_t, umode_t);
    int (*rmdir)(path_t);
    int (*rename)(path_t, path_t);
    int (*getattr)(path_t, struct stat *);
};

struct file_system
{
    const char *name;
    const struct fs_operations *fs_op;
};

extern int vfilesys_init();

/**
 * @return first unmatched position
 */
static inline int str_match_prefix(const char *str, const char *prefix)
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

/**
 * Convert relative path to full path, remove all "." and ".."
 * use this only when path is NOT started with "/"
 * @path: path to convert
 * @full_path: buffer to store full path, should be initialized by cwd
 * @return: length of full path on success, -1 on error
 */
static inline int fullpath_connect(const char *path, char *full_path)
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

static inline int get_absolute_path(const char *path, char *full_path, fd_t dirfd)
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

#endif // __FS_H__