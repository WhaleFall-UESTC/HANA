#include <sys/utsname.h>
#include <syscall.h>
#include <common.h>
#include <proc/proc.h>

SYSCALL_DEFINE1(uname, int, struct utsname*, buf) {
    return 0;
}