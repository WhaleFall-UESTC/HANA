#include <ulib.h>
#include <stddef.h>

// get filename from path
char *
fmtname(char *path)
{
    static char buf[32 + 1]; // suppose DIRSIZ 32
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= 32)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', 32 - strlen(p)); 
    return buf;
}

void ls(char *path)
{
    char buf[512]={0};
    int fd;
    struct stat st;

    printf("ls %s\n", path);

    if ((fd = openat(AT_FDCWD, path, O_RDONLY | O_DIRECTORY, 0)) < 0)
    {
        printf("ls: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        printf("ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if (S_ISREG(st.st_mode))
    {
        printf("%s %lu %ld\n", fmtname(path), (unsigned long)st.st_ino, st.st_size);
    }
    else if (S_ISDIR(st.st_mode))
    {
        if (strlen(path) + 1 + 32 + 1 > sizeof buf)
        {
            printf("ls: path too long\n");
        }
        else
        {
            for (;;)
            {
                long nread = getdents64(fd, (struct dirent *)buf, 512);
                if (nread == -1)
                {
                    printf("ls: getdents64\n");
                    break;
                }
                if (nread == 0)
                    break; // 目录读完了

                for (long pos = 0; pos < nread;)
                {
                    struct dirent *d = (struct dirent*)(buf + pos);

                    printf("%s", d->d_name);

                    /* 根据 d_type 显示类型（可选） */
                    switch (d->d_type)
                    {
                    case 4:
                        puts("  [dir]");
                        break; // DT_DIR
                    case 8:
                        puts("  [file]");
                        break; // DT_REG
                    case 10:
                        puts("  [link]");
                        break; // DT_LNK
                    default:
                        puts("");
                        break;
                    }

                    pos += d->d_reclen; // 关键：跳到下一个条目
                }
            }
        }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2)
    {
        ls(".");
        exit(0);
    }
    for (i = 1; i < argc; i++)
        ls(argv[i]);
    exit(0);
}