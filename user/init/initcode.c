#include "../include/syscall.h"

char arg0[32] = "/glibc/busybox";
char arg1[32] = "/glibc/basic_testcode.sh";
// char arg2[32] = "/glibc/basic_test.sh";
// char arg3[32] = "/glibc/basic_.sh";

// char env1[32] = "/glibc/basic.sh";
// char env2[32] = "/glibc";

char* const argv[] = { arg0, arg1, 0 };
// char* const envp[] = { env1, env2, 0 };


int main(void) {
    mount("/dev/sda", "/", "ext4", 0, 0);
    // execve("/la_init", 0, 0);

    const char busybox[] = "/glibc/busybox";
    
    execve(busybox, argv, 0);

    return 0;
}