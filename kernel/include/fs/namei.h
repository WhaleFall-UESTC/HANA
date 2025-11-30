#ifndef __NAMEI_H__
#define __NAMEI_H__

#include <fs/path.h>

struct bstr {
	int len;
	const char * str;
};

enum {LAST_NORM, LAST_ROOT, LAST_DOT, LAST_DOTDOT};

#define INNER_STACK_SIZE 4
static struct nameidata {
	struct path path; /* the path current component related to */
	struct inode * inode; /* path.dentry.d_inode */

	struct bstr * last; /* name of last got component */
	int last_type;
	struct path root;
	// unsigned int flags; // open flag of 

	struct saved {
		struct path link;
		const char * name;
	} *stack, internel[INNER_STACK_SIZE];
	int depth;
};

#endif // __NAMEI_H__