#include "../include/syscall.h"
#include "../include/ulib.h"

int main() {
    printf("mount\n");
    mount("/dev/sda", "/", "ext4", 0, 0);
    printf("mounted\n");

    // char** echo_args = (char*[]) { "echo", "Hello, World!", 0 };
    // execve("echo", echo_args, null);

    char **ls_args = (char *[]){"ls", "/", 0};
    execve("ls", ls_args, null);

    while(1);
}

// int main(void) {
//     mount("/dev/sda", "/", "ext4", 0, 0);
//     // execve("/la_init", 0, 0);

//     chdir("/glibc");

//     const char busybox[] = "busybox";

//     char arg0[32] = "busybox";
//     char arg1[32] = "basic_testcode.sh";
    
//     puts(arg0);
//     puts(arg1);

//     char* argv[] = { arg0, arg1, 0 };

//     printf("arg0 addr 0x%p, arg1 addr 0x%p\n", arg0, arg1);
//     printf("argv addr 0x%p\n", argv);

//     execve(busybox, argv, 0);

//     return 0;
// }