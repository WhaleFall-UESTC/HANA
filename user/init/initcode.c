#include "../include/syscall.h"

int main(void) {
    mount("/dev/sda", "/", "ext4", 0, 0);
    // execve("/la_init", 0, 0);

    const char busybox[] = "/glibc/busybox";
    
    char arg1[32] = "/glibc/basic_testcode.sh";

    char* const argv[2] = { arg1, 0 };

    execve(busybox, argv, 0);

    return 0;
}