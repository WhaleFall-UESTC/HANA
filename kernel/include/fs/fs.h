#ifndef __FS_H__
#define __FS_H__

#include <common.h>
#include <locking/spinlock.h>
#include <io/blk.h>
#include <fs/stat.h>
#include <fs/inode.h>

#define MAX_PATH_LEN 1024
#define MAX_FILENAME_LEN 256
typedef const char *path_t;

struct file;
struct mountpoint;

struct fs_operations
{
    int (*mount)(struct blkdev *, struct mountpoint *, const char *);
    int (*umount)(struct mountpoint *);
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

/**
 * Initialize the virtual file system
 * This function should be called before any file system operations
 * @return: 0 on success, -1 on error
 */
extern int vfilesys_init();

/**
 * Check if a string starts with a given prefix
 * @str: the string to check
 * @prefix: the prefix to check against
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
int fullpath_connect(const char *path, char *full_path);

/**
 * Get absolute path from relative path
 * @path: relative path to convert
 * @full_path: buffer to store absolute path, should be initialized by cwd
 * @dirfd: directory file descriptor, used to resolve relative path
 * @return: 0 on success, -1 on error
 */
int get_absolute_path(const char *path, char *full_path, fd_t dirfd);

/**
 * Find a file system by its type
 * @param fstype: file system type string
 * @return: pointer to the file_system struct if found, NULL otherwise
 */
const struct file_system *filesys_find(const char *fstype);

#endif // __FS_H__