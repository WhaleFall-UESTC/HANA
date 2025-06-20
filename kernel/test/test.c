#include <common.h>
#include <testdefs.h>
#include <debug.h>
#include <arch.h>

extern void exit(int);

void test() {
    // intr_off();
    PASS("test start!!!");

    lab_cow();

    // test_slab();
    // test_kalloc();


    // test_uart();
    // test_virtio();
    // test_walkaddr();
    // test_virtio();

    test_fs();

    PASS("test finish!!!");
    // intr_on();
    exit(-1);
}