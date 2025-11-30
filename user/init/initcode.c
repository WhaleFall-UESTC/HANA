#include "../include/syscall.h"
#include "../include/ulib.h"

int main() {
    printf("mount\n");
    mount("/dev/sda", "/", "ext4", 0, 0);
    printf("mounted\n");

    // int pid = clone(0, 0, 0, 0, 0);
    // if (pid == 0) {
    //     char** echo_args = (char*[]) { "echo", "Hello, World!", 0 };
    //     execve("echo", echo_args, null);
    // }
    // int status;
    // wait4(pid, &status, 0, null);

    // char **cat_args = (char *[]){"cat", "/data/bad-apple-ascii/out0140.jpg.txt", 0};
    // // char **cat_args = (char *[]){"cat", "/data/text", 0};
    // execve("cat", cat_args, null);

    char **ls_args = (char *[]){"ls", "/", 0};
    execve("ls", ls_args, null);

    while(1);
}
