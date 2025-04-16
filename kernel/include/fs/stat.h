#ifndef __STAT_H__
#define __STAT_H__

struct kstat {
  uint64 st_dev;
  uint64 st_ino;
  uint32 st_mode;
  uint32 st_nlink;
  uint32 st_uid;
  uint32 st_gid;
  uint64 st_rdev;
  uint16 __pad2;
  uint64 st_size;
  uint32 st_blksize;
  uint64 st_blocks;
  uint64 st_atime;
  uint64 st_atimensec;
  uint64 st_mtime;
  uint64 st_mtimensec;
  uint64 st_ctime;
  uint64 st_ctimensec;
  uint64 __unused[2];
};

struct statfs {
  uint64 f_type;
  uint64 f_bsize;
  uint64 f_blocks;
  uint64 f_bfree;
  uint64 f_bavail;

  uint64 f_files;
  uint64 f_ffree;
  struct {
    int val[2];
  } f_fsid;
  uint64 f_namelen;
  uint64 f_frsize;
  uint64 f_flags;

  uint64 f_spare[4];
};

#endif