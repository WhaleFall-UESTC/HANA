#include "../include/syscall.h"
#include "../include/ulib.h"

// char arg2[32] = "/glibc/basic_test.sh";
// char arg3[32] = "/glibc/basic_.sh";

// char env1[32] = "/glibc/basic.sh";
// char env2[32] = "/glibc";

// char* const envp[] = { env1, env2, 0 };


int main(void) {
    mount("/dev/sda", "/", "ext4", 0, 0);
    // execve("/la_init", 0, 0);

    chdir("/glibc");

    const char busybox[] = "busybox";

    char arg0[32] = "busybox";
    char arg1[32] = "basic_testcode.sh";
    
    puts(arg0);
    puts(arg1);

    char* argv[] = { arg0, arg1, 0 };

    printf("arg0 addr 0x%p, arg1 addr 0x%p\n", arg0, arg1);
    printf("argv addr 0x%p\n", argv);

    execve(busybox, argv, 0);

    return 0;
}