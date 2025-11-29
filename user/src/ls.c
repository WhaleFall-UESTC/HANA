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
    char buf[128]={0}, *p;
    char filename[32]={0};
    int fd;
    struct dirent de;
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
            strcpy(buf, path);
            p = buf + strlen(buf);
            if (*(p-1) != '/') {
                *p++ = '/';
                *p = '\0';
            }

            while (read(fd, &de, SIZE_PEEK) == SIZE_PEEK)
            {
                if (de.d_ino == 0)
                    continue;

                printf("Got dirent: ino=%lu, off=%ld, reclen=%u, type=%u\n",
                       de.d_ino, de.d_off, de.d_reclen, de.d_type);

                unsigned long filename_bytes = de.d_reclen - SIZE_PEEK;
                filename_bytes = (filename_bytes < 32) ? filename_bytes : 32;
                memset(filename, 0, 32);
                if (read(fd, filename, filename_bytes) != filename_bytes) {
                    printf("ls: read filename failed\n");
                    break;
                }
                printf("Filename: %s\n", filename);

                memmove(p, filename, strlen(filename));
                p[strlen(filename)] = '\0';
                printf("Processing %s\n", buf);

                int child_fd = openat(AT_FDCWD, buf, O_RDONLY, 0);
                if (child_fd < 0)
                {
                    printf("ls: cannot open %s\n", buf);
                    continue;
                }

                struct stat child_st;
                if (fstat(child_fd, &child_st) < 0)
                {
                    printf("ls: cannot stat %s\n", buf);
                    close(child_fd);
                    continue;
                }

                printf("%s %lu %ld\n",
                       fmtname(buf),
                       (unsigned long)child_st.st_ino,
                       child_st.st_size);

                close(child_fd);
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