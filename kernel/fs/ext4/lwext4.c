// #include <fs/ext4/lwext4/ext4_config.h>
// #include <fs/ext4/lwext4/ext4_types.h>
// #include <fs/ext4/lwext4/ext4_misc.h>
// #include <fs/ext4/lwext4/ext4_errno.h>
// #include <fs/ext4/lwext4/ext4_oflags.h>
// #include <fs/ext4/lwext4/ext4_debug.h>

// #include <fs/ext4/lwext4/ext4.h>
// #include <fs/ext4/lwext4/ext4_trans.h>
// #include <fs/ext4/lwext4/ext4_blockdev.h>
// #include <fs/ext4/lwext4/ext4_fs.h>
// #include <fs/ext4/lwext4/ext4_dir.h>
// #include <fs/ext4/lwext4/ext4_inode.h>
// #include <fs/ext4/lwext4/ext4_super.h>
// #include <fs/ext4/lwext4/ext4_block_group.h>
// #include <fs/ext4/lwext4/ext4_dir_idx.h>
// #include <fs/ext4/lwext4/ext4_xattr.h>
// #include <fs/ext4/lwext4/ext4_journal.h>

// #include <klib.h>

// static int ext4_generic_open2_inode(ext4_file *f, uint32 inode, const char *path, int flags,
// 			      int ftype, uint32 *parent_inode,
// 			      uint32 *name_off)
// {
// 	bool is_goal = false;
// 	uint32 imode = EXT4_INODE_MODE_DIRECTORY;
// 	uint32 next_inode;

// 	int r;
// 	int len;
// 	struct ext4_mountpoint *mp = ext4_get_mount(path);
// 	struct ext4_dir_search_result result;
// 	struct ext4_inode_ref ref;

// 	f->mp = 0;

// 	if (!mp)
// 		return ENOENT;

// 	struct ext4_fs *const fs = &mp->fs;
// 	struct ext4_sblock *const sb = &mp->fs.sb;

// 	if (fs->read_only && flags & O_CREAT)
// 		return EROFS;

// 	f->flags = flags;

// 	/*Skip mount point*/
// 	path += strlen(mp->name);

// 	if (name_off)
// 		*name_off = strlen(mp->name);

// 	/*Load cwd*/
// 	r = ext4_fs_get_inode_ref(fs, inode, &ref);
// 	if (r != EOK)
// 		return r;

// 	if (parent_inode)
// 		*parent_inode = ref.index;

// 	len = ext4_path_check(path, &is_goal);
// 	while (1) {

// 		len = ext4_path_check(path, &is_goal);
// 		if (!len) {
// 			/*If root open was request.*/
// 			if (ftype == EXT4_DE_DIR || ftype == EXT4_DE_UNKNOWN)
// 				if (is_goal)
// 					break;

// 			r = ENOENT;
// 			break;
// 		}

// 		r = ext4_dir_find_entry(&result, &ref, path, len);
// 		if (r != EOK) {

// 			/*Destroy last result*/
// 			ext4_dir_destroy_result(&ref, &result);
// 			if (r != ENOENT)
// 				break;

// 			if (!(f->flags & O_CREAT))
// 				break;

// 			/*O_CREAT allows create new entry*/
// 			struct ext4_inode_ref child_ref;
// 			r = ext4_fs_alloc_inode(fs, &child_ref,
// 					is_goal ? ftype : EXT4_DE_DIR);

// 			if (r != EOK)
// 				break;

// 			ext4_fs_inode_blocks_init(fs, &child_ref);

// 			/*Link with root dir.*/
// 			r = ext4_link(mp, &ref, &child_ref, path, len, false);
// 			if (r != EOK) {
// 				/*Fail. Free new inode.*/
// 				ext4_fs_free_inode(&child_ref);
// 				/*We do not want to write new inode.
// 				  But block has to be released.*/
// 				child_ref.dirty = false;
// 				ext4_fs_put_inode_ref(&child_ref);
// 				break;
// 			}

// 			ext4_fs_put_inode_ref(&child_ref);
// 			continue;
// 		}

// 		if (parent_inode)
// 			*parent_inode = ref.index;

// 		next_inode = ext4_dir_en_get_inode(result.dentry);
// 		if (ext4_sb_feature_incom(sb, EXT4_FINCOM_FILETYPE)) {
// 			uint8 t;
// 			t = ext4_dir_en_get_inode_type(sb, result.dentry);
// 			imode = ext4_fs_correspond_inode_mode(t);
// 		} else {
// 			struct ext4_inode_ref child_ref;
// 			r = ext4_fs_get_inode_ref(fs, next_inode, &child_ref);
// 			if (r != EOK)
// 				break;

// 			imode = ext4_inode_type(sb, child_ref.inode);
// 			ext4_fs_put_inode_ref(&child_ref);
// 		}

// 		r = ext4_dir_destroy_result(&ref, &result);
// 		if (r != EOK)
// 			break;

// 		/*If expected file error*/
// 		if (imode != EXT4_INODE_MODE_DIRECTORY && !is_goal) {
// 			r = ENOENT;
// 			break;
// 		}
// 		if (ftype != EXT4_DE_UNKNOWN) {
// 			bool df = imode != ext4_fs_correspond_inode_mode(ftype);
// 			if (df && is_goal) {
// 				r = ENOENT;
// 				break;
// 			}
// 		}

// 		r = ext4_fs_put_inode_ref(&ref);
// 		if (r != EOK)
// 			break;

// 		r = ext4_fs_get_inode_ref(fs, next_inode, &ref);
// 		if (r != EOK)
// 			break;

// 		if (is_goal)
// 			break;

// 		path += len + 1;

// 		if (name_off)
// 			*name_off += len + 1;
// 	}

// 	if (r != EOK) {
// 		ext4_fs_put_inode_ref(&ref);
// 		return r;
// 	}

// 	if (is_goal) {

// 		if ((f->flags & O_TRUNC) && (imode == EXT4_INODE_MODE_FILE)) {
// 			r = ext4_trunc_inode(mp, ref.index, 0);
// 			if (r != EOK) {
// 				ext4_fs_put_inode_ref(&ref);
// 				return r;
// 			}
// 		}

// 		f->mp = mp;
// 		f->fsize = ext4_inode_get_size(sb, ref.inode);
// 		f->inode = ref.index;
// 		f->fpos = 0;

// 		if (f->flags & O_APPEND)
// 			f->fpos = f->fsize;
// 	}

// 	return ext4_fs_put_inode_ref(&ref);
// }