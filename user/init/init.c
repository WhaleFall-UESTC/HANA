#include <stddef.h>
#include "../include/syscall.h"
#include "../include/ulib.h"

size_t 
strlen(const char *s)
{
    int i;
    for (i = 0; s[i] != '\0'; i++)
        ;
    return i;
}

int main(int argc, char* const argv[], char* const envp[]) {
    // printf("argc: %d", argc);
    write(1, argv[0], strlen(argv[0]));
    write(1, "\n", 1);
    write(1, argv[1], strlen(argv[1]));
    write(1, "\n", 1);
    write(1, argv[2], strlen(argv[2]));
    write(1, "\n", 1);
    write(1, envp[0], strlen(envp[0]));
    write(1, "\n", 1);
    write(1, envp[1], strlen(envp[1]));
    write(1, "\n", 1);

    char *msg = "Hello, World!\n";
    write(1, msg, strlen(msg));
    return 0;
}
