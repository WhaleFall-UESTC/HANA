#include <ulib.h>

char buf[64];

void cat(int fd)
{
    ssize_t n;

    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        if (write(1, buf, n) != n)
        {
            printf("cat: write error\n");
            exit(1);
        }
    }
    if ((long)n < 0)
    {
        printf("cat: read error\n");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    int fd, i;

    if (argc <= 1)
    {
        cat(0);
        exit(0);
    }

    for (i = 1; i < argc; i++)
    {
        if ((fd = openat(AT_FDCWD, argv[i], O_RDONLY, 0)) < 0) 
        {
            printf("cat: cannot open %s\n", argv[i]);
            exit(1);
        }
        cat(fd);
        close(fd);
    }
    exit(0);
}