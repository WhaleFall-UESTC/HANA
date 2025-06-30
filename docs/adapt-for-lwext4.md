# 适配文件系统库：lwext4

HANAOS 实现了优秀的文件系统兼容层和硬件抽象层，故实现第三方文件系统库的适配是比较容易的。

## struct ext4_fs_dev

```c
struct ext4_fs_dev {
    struct ext4_blockdev ext4_blkdev;
    struct ext4_blockdev_iface ext4_blkdev_if;
    struct blkdev *blkdev;
    const char* name;
};
```

HANAOS 将 lwext4 注册和挂载需要的数据以字段的形式保存在`struct ext4_fs_dev`中，当我们实现的 api 获得某一字段的地址时，可以方便的访问到其他字段。

## 文件操作接口

HANAOS 实现了`struct file_operations ext4_file_fops`和`struct fs_operations ext4_filesystem_ops`作为文件接口和文件系统接口集，二者包含的函数如下：

```c
const struct file_operations ext4_file_fops = {
	.llseek = ext4_llseek,
	.read = ext4_read,
	.write = ext4_write,
	.openat = ext4_openat,
	.close = ext4_close,
	.getdents64 = ext4_getdents64,
	.truncate = ext4_truncate,
};

const struct fs_operations ext4_filesystem_ops = {
	.mount = ext4_fs_mount,
	.umount = ext4_fs_umount,
	.ifget = ext4_fs_ifget,
	.link = ext4_link,
	.unlink = ext4_unlink,
	.symlink = ext4_symlink,
	.mkdir = ext4_mkdir,
	.rmdir = ext4_rmdir,
	.rename = ext4_rename,
	.getattr = ext4_getattr,
};
```

例如，文件系统接口`mount`的对应函数`ext4_fs_mount`，该函数根据 lwext4 官方的注册示例，将底层 I/O 接口等信息填入`struct ext4_fs_dev`，并依次调用注册和挂载所需的 lwext4 函数。

因为文件打开之后，很多 lwext4 函数需要`struct ext4_file`等 lwext4 数据结构，HANAOS 将这些数据结构保存在`struct file`的 `f_private`字段。

除此之外的注册的函数基本为对应 lwext4 函数的 wrapper，此处不再赘述。

## 底层 I/O 接口

HANAOS 实现了 lwext4 官方示例中的几个标准接口：

```c
int blockdev_open(struct ext4_blockdev *bdev);
int blockdev_bread(struct ext4_blockdev *bdev, void *buf, uint64 blk_id,
			 uint32 blk_cnt);
int blockdev_bwrite(struct ext4_blockdev *bdev, const void *buf,
			  uint64 blk_id, uint32 blk_cnt);
int blockdev_close(struct ext4_blockdev *bdev);
int blockdev_lock(struct ext4_blockdev *bdev);
int blockdev_unlock(struct ext4_blockdev *bdev);
```

这些接口会将 I/O 请求或者其他操作转发给`bdev`所属的`struct ext4_fs_dev`中注册的块设备，并等待操作完成。