#ifndef __FCNTL_H__
#define __FCNTL_H__

#define O_ACCMODE	00000003
#define O_RDONLY	00000000
#define O_WRONLY	00000001
#define O_RDWR		00000002
#define O_CREAT		00000100
#define O_EXCL		00000200
#define O_NOCTTY	00000400
#define O_TRUNC		00001000
#define O_APPEND	00002000
#define O_NONBLOCK	00004000
#define O_DSYNC		00010000
#define O_DIRECT	00040000
#define O_LARGEFILE	00100000
#define O_DIRECTORY	00200000
#define O_NOFOLLOW	00400000	        /* don't follow links */
#define O_NOATIME	01000000
#define O_CLOEXEC	02000000	        /* set close_on_exec */

#define FD_CLOEXEC 1

#define AT_FDCWD		-100            /* Special value used to indicate
                                           openat should use the current
                                           working directory. */
#define AT_SYMLINK_NOFOLLOW	0x100       /* Do not follow symbolic links.  */
#define AT_EACCESS		0x200           /* Test access permitted for
                                           effective IDs, not real IDs.  */
#define AT_REMOVEDIR		0x200       /* Remove directory instead of
                                           unlinking file.  */
#define AT_SYMLINK_FOLLOW	0x400       /* Follow symbolic links.  */
#define AT_NO_AUTOMOUNT		0x800       /* Suppress terminal automount traversal */
#define AT_EMPTY_PATH		0x1000	    /* Allow empty relative pathname */

#define AT_STATX_SYNC_TYPE	0x6000      /* Type of synchronisation required from statx() */
#define AT_STATX_SYNC_AS_STAT	0x0000  /* - Do whatever stat() does */
#define AT_STATX_FORCE_SYNC	0x2000	    /* - Force the attributes to be sync'd with the server */
#define AT_STATX_DONT_SYNC	0x4000	    /* - Don't sync attributes with the server */

#define AT_RECURSIVE		0x8000	    /* Apply to the entire subtree */

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_DUPFD_CLOEXEC 1030

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define	F_OK	0
#define	R_OK	4
#define	W_OK	2
#define	X_OK	1

#endif // __FCNTL_H__