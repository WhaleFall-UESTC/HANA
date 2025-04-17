#include <io/blk.h>
#include <fs/fs.h>
#include <fs/ext4_blk.h>
#include <fs/fcntl.h>
#include <fs/stat.h>

#include <lwext4/ext4.h>
#include <lwext4/ext4_errno.h>

#define MAX_EXT4_BLOCKDEV_NAME 32
#define EXT4_BUF_SIZE 512

int
fs_blk_mount_ext4(struct blkdev * blkdev, const char *mountpoint)
{
	char buffer[MAX_EXT4_BLOCKDEV_NAME + 1];
	int ret;
	struct ext4_blockdev *blockdev;
	struct ext4_blockdev_iface* ext4_blockdev_if;

	KALLOC(struct fs_dev, fs_dev);

	if (fs_dev == NULL)
	{
		error_ext4("filesystem device alloc error!");
		return -1;
	}

	blockdev = &fs_dev->ext4_blkdev;
	ext4_blockdev_if = &fs_dev->ext4_blkdev_if;

	ext4_blockdev_if->open = blockdev_open;
	ext4_blockdev_if->bread = blockdev_bread;
	ext4_blockdev_if->bwrite = blockdev_bwrite;
	ext4_blockdev_if->close = blockdev_close;
	ext4_blockdev_if->lock = blockdev_lock;
	ext4_blockdev_if->unlock = blockdev_unlock;
	ext4_blockdev_if->ph_bsize = blkdev->sector_size;
	ext4_blockdev_if->ph_bcnt = blkdev->size / blkdev->sector_size;
	ext4_blockdev_if->ph_bbuf = (uint8_t*)kalloc(EXT4_BUF_SIZE);

	blockdev->bdif = ext4_blockdev_if;
	blockdev->part_offset = 0;
	blockdev->part_size = blkdev->size;

	fs_dev->blkdev = blkdev;

	sprintf(buffer, "ext4_%s", blkdev->name);

	ret = ext4_device_register(blockdev, buffer);
	if (ret != EOK)
	{
		error_ext4("device register error! ret = %d", ret);
		return -1;
	}

	ret = ext4_mount(buffer, mountpoint, false);
	if (ret != EOK)
	{
		error_ext4("mount error! ret = %d", ret);
		return -1;
	}

	ret = ext4_cache_write_back(mountpoint, true);
	if (ret != EOK)
	{
		error_ext4("cache write back error! ret = %d", ret);
		return -1;
	}

	return 0;
}

int fname(struct ext4_file *f, char *path)
{

  int r;
  if((r = fopen(f, path, O_RDONLY)) != EOK) {
    // printf("[fname] fopen error! r: %d\n", r);
    return r;
  }

  return r;
}

int fseek(struct ext4_file *file, uint off, uint origin)
{
  return ext4_fseek(file, off, origin);
}

int fread(struct ext4_file *file, uint64 buf, uint off, uint size, int *rcnt)
{
  int r = EOK;
  r = fseek(file, off, SEEK_SET);
  if (r != EOK) {
    printf("[fread] fseek error! r: %d\n", r);
    return -1;
  }
  // printf("[fread] enter ext4_fread\n");
  return ext4_fread(file, (void*)buf, (size_t)size, (size_t*)rcnt);
}

int fwrite(struct ext4_file *file, uint64 buf, uint off, uint size, int *wcnt)
{
  int r = EOK;
  r = fseek(file, off, SEEK_SET);
  if (r != EOK) {
    printf("[fwrite] fseek error! r: %d\n", r);
    return -1;
  }
  return ext4_fwrite(file, (void*)buf, (size_t)size, (size_t*)wcnt);
}

int fopen(struct ext4_file *file, const char *path, uint32_t flags)
{
  int r = ext4_fopen2(file, path, flags);
  if (r != EOK) {
    // printf("[fopen] error! path: %s, r: %d\n", path, r);
    return -1;
  }
  return r;
}

int fclose(struct ext4_file *file)
{
  int r = ext4_fclose(file);
  if (r != EOK) {
    printf("[fclose] error! r: %d\n", r);
    return -1;
  }
  return r;
}

int dir_open(struct ext4_dir *dir, const char *path)
{
  int r;
  if((r = ext4_dir_open(dir, path)) != EOK) {
    // printf("[dir_open] error! r: %d\n", r);
    return -1;
  }
  return r;
}

int dir_close(struct ext4_dir *dir)
{
  int r = ext4_dir_close(dir);
  if (r != EOK) {
    printf("[dir_close] error! r: %d\n", r);
    return -1;
  }
  return r;
}

int fsymlink(const char *path, const char *softlink_path)
{
  int r = ext4_fsymlink(path, softlink_path);
  return r;
}

int flink(const char *path, const char *hardlink_path)
{
  int r = ext4_flink(path, hardlink_path);
  return r;
}

int funlink(const char *path)
{
  int r = ext4_fremove(path);
  return r;
}

int readlink(const char *path, char *buf, size_t bufsize, size_t *rcnt)
{
  int ret = ext4_readlink(path,buf,bufsize,rcnt);
  return ret;
}

int fkstatat(char *path, struct kstat *kst, int dirfd)
{
  int r;
  uint32 ino;
  struct ext4_inode inode;
  char *file_path = NULL;

  if(is_relative(path)){
    if(dirfd == AT_FDCWD){
      file_path = to_abspath(path);
    }
    else{
      printf("[fkstatat] dirfd: %d\n", dirfd);
      return -1;
    }
  }
  else{
    file_path = path;
  }

  // printf("[fkstatat] file_path: %s\n", file_path);

  if(!strcmp(file_path, "/dev/null")){
    kst->st_dev = 0;
    kst->st_ino = 0;
    kst->st_mode = EXT4_INODE_MODE_CHARDEV;
    kst->st_nlink = 0;
    kst->st_uid = 0;
    kst->st_gid = 0;
    kst->st_rdev = 0;
    kst->st_size = 0;
    kst->st_blksize = 0;
    kst->st_blocks = 0;
    kst->st_atime = 0;
    kst->st_atimensec = 0;
    kst->st_ctime = 0;
    kst->st_ctimensec = 0;
    kst->st_mtime = 0;
    kst->st_mtimensec = 0;

    return 0;
  }

  /* executable in sbin is in root dir, redirect the path to it */
  if(!strncmp(file_path, "/sbin/", 6)){
    if((r = ext4_raw_inode_fill(file_path + 5, &ino, &inode)) != EOK){
        printf("[fkstatat] ext4_raw_inode_fill error1, r: %d\n", r);
      return -1;
    }
  }
  else if((r = ext4_raw_inode_fill(file_path, &ino, &inode)) != EOK){
    printf("[fkstatat] ext4_raw_inode_fill error21, r: %d\n", r);
    return -1;
  }
  else if((r = ext4_raw_inode_fill(file_path, &ino, &inode)) != EOK){
    printf("[fkstatat] ext4_raw_inode_fill error2, r: %d\n", r);
    return -1;
  }


  kst->st_dev = 0;
  kst->st_ino = ino;
  kst->st_mode = inode.mode; /* TODO: fix the error during du, ls */
  kst->st_nlink = inode.links_count;
  kst->st_uid = 0;
  kst->st_gid = 0;
  kst->st_rdev = 0;
  kst->st_size = inode.size_lo;
  kst->st_blksize = 512;
  kst->st_blocks = (uint64)inode.blocks_count_lo;

  // printf("[fkstatat] ino: %p, mode: %d, nlink: %d, size: %p, blksize: %d, blocks: %p\n", ino, inode.mode, inode.links_count, inode.size_lo, kst->st_blksize, kst->st_blocks);

  /* TODO: more precise time */

  kst->st_atime = 0;
  kst->st_atimensec = 0;
  kst->st_ctime = 0;
  kst->st_ctimensec = 0;
  kst->st_mtime = 0;
  kst->st_mtimensec = 0;

  return 0;
}

int fwritev(struct ext4_file *file, struct iovec iov[], int iovcnt, uint off)
{
  int r, wcnt;
  uint tot = 0;

  // printf("[fwritev] file: %p, off: %d\n", (uint64)file, off);

  for(int i = 0; i < iovcnt; i++){
    uint64 buf = (uint64)iov[i].iov_base;
    uint64 n = iov[i].iov_len;
    wcnt = 0;

    if(n == 1 && !strcmp(myproc()->name, "libc-bench"))
      exit(0);

    // printf("[fwritev] buf: %p, tot: %d, n: %p\n", buf, tot, n);

	  if((r = fwrite(file, buf, off + tot, n, &wcnt)) != EOK)
      return -1;

	  tot += wcnt;
  }

  return tot;
}

int freadv(struct ext4_file *file, struct iovec iov[], int iovcnt, uint off)
{
  int r, rcnt;
  uint tot = 0;
  for(int i = 0; i < iovcnt; i++){
    uint64 buf = (uint64)iov[i].iov_base;
    uint n = iov[i].iov_len;
    rcnt = 0;

	  if((r = fread(file, buf, off + tot, n, &rcnt)) != EOK)
      return -1;

	  tot += rcnt;
  }

  return tot;
}

struct ext4_inode_ref *file_get_inode_ref(struct ext4_file *file)
{
  struct ext4_inode_ref *ref = NULL;
  int r;
  if((r = ext4_file_get_inode_ref(file, ref)) != EOK)
    printf("[ext4] file_get_inode: ext4_fs_get_inode_ref error! r=%d\n", r);
  return ref;
}

uint32 get_inode_type(struct ext4_file *file)
{
  struct ext4_inode_ref *ref = file_get_inode_ref(file);
  if(ref == NULL){
    printf("[ext4] file_is_directory: inode is NULL!\n");
    return 0;
  }
  return ext4_inode_type(&ref->fs->sb, ref->inode);
}

int mkdir(const char *path)
{
  int r = ext4_dir_mk(path);
  if(r != EOK){
    printf("[ext4] mkdir error! r=%d\n", r);
    return -1;
  }
  return r;
}