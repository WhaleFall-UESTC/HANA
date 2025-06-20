#include <common.h>
#include <arch.h>
#include <mm/mm.h>
#include <mm/memlayout.h>
#include <debug.h>

extern char trampoline[];

void lab_cow() {
    uint64 v = *(uint64*) TEST_SPACE;
    log("v: %lx", v);
    *(uint64*) TEST_SPACE = v;
}