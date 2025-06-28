#include "../include/syscall.h"

int main(void) {
    mount("/dev/sda", "/", "ext4", 0, 0);
    execve("/la_init", 0, 0);
    return 0;
}