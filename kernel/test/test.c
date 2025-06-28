#include <common.h>
#include <testdefs.h>
#include <debug.h>
#include <arch.h>

extern void do_exit(int);

void test() {
    // intr_off();
    PASS("test start!!!");

    test_execve();
    // test_fs();

    PASS("test finish!!!");
    // intr_on();
    do_exit(-1);
}