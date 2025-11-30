#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/stat.h>
#include <fs/dirent.h>
#include <fs/inode.h>
#include <fs/dcache.h>
#include <fs/namei.h>
#include <proc/proc.h>
#include <debug.h>
#include <lib/errno.h>

static inline int bstrcmp(struct bstr* s1, struct bstr* s2) {
	return (s1->len - s2->len) + (s1->len == s2->len ? strncmp(s1->str, s2->str, s1->len) : 0);
}

static struct dentry * lookup_in_cache(struct nameidata *nd) {
	struct dentry * dent;

	list_for_each_entry(dent, &nd->path.dentry->d_subdirs, d_child) {
		if(nd->last->len != strlen(dent->d_name) || strncmp(nd->last->str, dent->d_name, nd->last->len))
			continue;

		return dent;
	}

	return NULL;
}

static struct dentry * lookup_in_fs(struct bstr * name, struct dentry * dir) {
	struct inode * inode = dir->d_inode;

	if (!inode->i_ops->lookup) {
		error("inode has no lookup operation");
		return NULL;
	}
	
	struct dentry * dentry = dentry_alloc(name->str, name->len, NULL, dir);
	inode->i_ops->lookup(inode, dentry, 0);
	
	if(!dentry->d_inode) {
		kfree(dentry);
		return NULL;
	}

	// add to cache
	list_add_tail(&dentry->d_child, &dir->d_subdirs);

	return dentry;
}

static const char * handle_dots(struct nameidata * nd) {
	switch(nd->last_type) {
		case LAST_ROOT:
			nd->path = nd->root;
			break;
		case LAST_DOT:
			break;
		case LAST_DOTDOT:
			nd->path = parent_path(&nd->path);
			break;
		default:
			error("Invalid last type.");
			return ERR_PTR(-EINVAL);
	}

	return NULL;
}

static const char * step_into(struct nameidata *nd, struct dentry * dentry, struct inode * inode) {
	const char * res;

	/**
	 * TODO: Judge by dentry flag
	 */
	if(S_ISLNK(inode->i_mode)) {
		// is symlink 
		struct saved *last;
		last = nd->stack + nd->depth;
		last->link = nd->path;

		res = inode->i_link;
		if(!res) {
			// try to get link
			res = inode->i_ops->get_link(dentry, inode);
			if(IS_ERR(res)) {
				error("Cannot get link");
				return res;
			}

			if(!res) return NULL;

			inode->i_link = res;
		}
	
		if(*res == '/') {
			nd->path = nd->root;
			while(*res + 1 == '/') res ++; // start of nd->name will start with new component
		}
	}
	else {
		nd->path.mp = nd->path.mp;
		nd->path.dentry = dentry;

		return NULL;
	}
}

/**
 * Follow single component that just found.
 * @return: content of the symlink if it is, otherwise NULL.
 */
static const char * walk_component(struct nameidata *nd) {
	struct dentry * dent = NULL;

	if(nd->last_type != LAST_NORM)
		return handle_dots(nd);

	// lookup in cache
	dent = lookup_in_cache(nd);
	if (!dent) {
		// lookup in fs
		dent = lookup_in_fs(nd->last, nd->path.dentry);
		if (!dent) return NULL;
	}

	return step_into(nd, dent, dent->d_inode);
}

void set_nameidata(struct nameidata * nd, const char * filepath, fd_t dfd) {
	struct files_struct *fdt;
	struct file * file;
	memset(nd, 0, sizeof(*nd));

	if(*filepath == '/') {
		nd->path = myproc()->root;
		nd->last_type = LAST_ROOT;
	}
	else {
		nd->last_type = LAST_NORM;
		if(dfd == AT_FDCWD) {
			nd->path = myproc()->cwd;
		}
		else {
			fdt = myproc()->fdt;
			file = fd_get(fdt, dfd);
			nd->path = file->f_path;
		}
	}

	nd->stack = nd->internel;
}

int link_path_walk(const char *name, struct nameidata *nd) {
    const char *start = name;
    struct inode *inode = nd->path.dentry->d_inode;
    struct bstr component;
	struct dentry * dent;

	while(1) {
		// skip repeat '/'
		while (*start == '/') start++;
		if (*start == '\0') {
			if(!nd->depth) return 0;
			
			start = nd->stack[--nd->depth].name;
		}
	
		// get current component
		const char *end = start;
		while (*end != '/' && *end != '\0') end++;
		component.len = end - start;
		component.str = start;
	
		// tackle special directory: . and ..
		if (component.len == 1 && *start == '.') {
			// current dir, just skip
			nd->last_type = LAST_DOT;
			goto next;
		} else if (component.len == 2 && memcmp(start, "..", 2) == 0) {
			// parent dir: check if is root
			nd->last_type = LAST_DOTDOT;
			goto next;
		}
	
		nd->last_type = LAST_NORM;
		
		char * link = walk_component(nd);
		if(IS_ERR(link)) {
			return PTR_ERR(link);
		}
		if(link) {
			if(nd->depth == INNER_STACK_SIZE) {
				error("Link count excceed max inner stack");
				return -EINVAL;
			}
			nd->stack[nd->depth++].name = start;
			start = link;
		}

next:
		start = end;
	}

    nd->inode = inode;
    nd->last = &(struct bstr){ .len = start - name, .str = name };
    return 0;
}

static int lookup_open(struct nameidata * nd, struct file * file, int flags) {
	
}

/**
 * Open last lookup result
 * If last looukup is not link and stack not empty, follow the stack
 * TODO: Change dir into hash list
 */
const char * open_last_lookups(struct nameidata *nd, struct file *file, unsigned int open_flag) {
	struct dentry* dent;
	struct inode *dir = nd->inode;
	const char * res = NULL;

	if(nd->last_type != LAST_NORM)
		return handle_dots(nd);

	// lookup in cache
	dent = lookup_in_cache(nd);
	if (!dent) {
		// lookup in fs
		dent = lookup_in_fs(nd->last, nd->path.dentry);
		if (!dent) return NULL;
	}

	res = step_into(nd, dent, dent->d_inode);

	if(!res && nd->depth) {
		res = nd->stack[nd->depth--].name;
	}
	
	return res;
}

struct dentry * vfs_open(const char * filename, fd_t dfd, int flags) {
	struct nameidata nd;
	struct file file;

	set_nameidata(&nd, filename, dfd);

	file_init(&file, NULL, NULL, 0, NULL);

	const char * s = filename;
	while((s = link_path_walk(s, &nd) != NULL)
	   && (s = open_last_lookups(&nd, &file, flags))) {
		if(IS_ERR(s)) {
			error("file lookup failed.");
			return (struct dentry *)s;
		}
	}


}