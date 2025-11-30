#include <fs/path.h>
#include <debug.h>
#include <lib/errno.h>

int path_str_fill(struct path * path, char * buffer, struct path * root) {
    char * cur = buffer + MAX_PATH_LEN, * start, * end;

    // start from tail
    while(path->dentry != root->dentry) {
        if(cur == buffer) {
            goto out_err;
        }

        start = path->dentry->d_name;
        end = start + MAX_FILENAME_LEN;
        while(start < end && *start && cur > buffer)
            *--cur = *start++;

        if(cur == buffer) {
            goto out_err;
        }

        *--cur = '/';
        *path = parent_path(path);
    }

    // move to head
    start = buffer;
    end = buffer + MAX_PATH_LEN;

    while(cur < end && start < end) {
        *start ++ = *cur ++;
    }

    return 0;

out_err:
    error("Path name too long");
    return -EINVAL;
}