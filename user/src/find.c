#include <ulib.h>
#include <fs.h>

#define BUFSIZ 128

#define EXIT(condition, info, ...)       \
    do                                   \
    {                                    \
        if (condition)                   \
        {                                \
            printf(info, ##__VA_ARGS__); \
            exit(1);                     \
        }                                \
    } while (0)

#define CHECK(exp, info, ...)        \
    if (exp)                         \
    {                                \
        printf(info, ##__VA_ARGS__); \
        return -1;                   \
    }

int checkpath(char *path, struct stat *st)
{
    int fd = 0;
    CHECK(strlen(path) + 1 + 256 + 1 > BUFSIZ, "find: path too long\n");
    CHECK((fd = openat(AT_FDCWD, path, O_RDONLY, 0)) < 0, "find: cannot open %s\n", path);
    CHECK(fstat(fd, st) < 0, "find: cannot stat %s\n", path);
    return fd;
}

int match(char *name1, char *name2)
{
    return strcmp(name1, name2);
}

void find(int fdpath, char *path, char *filename)
{
    struct dirent de;
    struct stat st;
    int fd;
    char buf[BUFSIZ];
    strcpy(buf, path);
    char *p = buf + strlen(buf);
    *p++ = '/';

    while (read(fdpath, &de, sizeof(de)) == sizeof(de))
    {
        if (de.d_ino == 0)
            continue;
        int name_len = strlen(de.d_name) < 256 ? strlen(de.d_name) : 255;
        memmove(p, de.d_name, name_len);
        p[name_len] = '\0';
        if ((fd = checkpath(buf, &st)) < 0 || !strcmp(de.d_name, ".") || !strcmp(de.d_name, ".."))
            continue;

        if (S_ISREG(st.st_mode))
        {
            if (match(de.d_name, filename) == 0)
                printf("%s\n", buf);
            close(fd);
        }
        else if (S_ISDIR(st.st_mode))
        {
            find(fd, buf, filename);
        }
    }
    close(fdpath);
}

int main(int argc, char *argv[])
{
    EXIT(argc != 3, "Usage: find path filename\n");
    char *path = argv[1];
    char *filename = argv[2];
    struct stat st;
    int fd;
    if ((fd = checkpath(path, &st)) < 0)
        exit(1);
    EXIT(!S_ISDIR(st.st_mode), "find: %s is not a directory\n", path);
    find(fd, path, filename);
    exit(0);
}