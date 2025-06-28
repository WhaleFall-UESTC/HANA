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

int main(void) {
    char *msg = "Hello, World!\n";
    write(1, msg, strlen(msg));
    return 0;
}
