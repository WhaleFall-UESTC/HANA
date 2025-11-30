#ifndef __PATH_H__
#define __PATH_H__

#include <fs/file.h>
#include <fs/mountp.h>
#include <fs/dcache.h>

struct path {
    struct mountpoint* mp;
    struct dentry* dentry;
};

// static inline path_t* path_alloc(struct mountpoint* mp, struct dentry* dent) {
//     KALLOC(path_t, path);
//     *path = 
// }

#define MAX_PATH_LEN 1024

static inline struct path parent_path(struct path * path) {
    return (struct path){
        .dentry = path->dentry->d_parent,
        .mp = path->dentry->d_inode->i_mp
    };
}

/**
 * Fill buffer with full path name
 * @param path Target path
 * @param buffer Target buffer, SHOULD be at least size of MAX_PATH_LEN
 * @param root Cur root
 * @return 0 for success, other for error
 */
int path_str_fill(struct path * path, char * buffer, struct path * root);

#endif // __PATH_H__