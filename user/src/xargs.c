#include <ulib.h>

#define BUFSIZ 128
#define MAXARG 10
#define STDIN_FILENO 0

#define EXIT(condition, info, ...)       \
    do                                   \
    {                                    \
        if (condition)                   \
        {                                \
            printf(info, ##__VA_ARGS__); \
            exit(1);                     \
        }                                \
    } while (0)

char *readline(int fd)
{
    char *buf = (char *)malloc(BUFSIZ);
    char *p = buf;
    do
    {
        if (read(fd, p, 1) == 0)
            return null;
    } while (*p++ != '\n');
    *(p - 1) = '\0';
    return buf;
}

int main(int argc, char *argv[])
{
    EXIT(argc < 2, "Usage: xargs command [arguments...]\n");
    char *exec_argv[MAXARG];
    exec_argv[0] = argv[1];
    for (int i = 2; i < argc; i++)
        exec_argv[i - 1] = argv[i];

    while ((exec_argv[argc - 1] = readline(STDIN_FILENO)) != null)
    {
        if (clone(0, 0, 0, 0, 0) == 0)     
            execve(argv[1], exec_argv, 0); 
        wait4(0, 0, 0, 0); 
    }
    exit(0);
}