#include <platform.h>
#include <memlayout.h>
#include <defs.h>
#include <debug.h>

#include <testdefs.h>

char init_stack[PGSIZE] __attribute__((aligned(PGSIZE)));
char *init_stack_top = &init_stack[PGSIZE];

extern char end[];

int 
main()
{
    uart_init();
    out("Initialize uart0");
    kinit((uint64) end, PHYSTOP);
    kvminit();
    out("Initialize vm");
    kvminithart();
    out("Enable paging");
    log("main return successfully");
    return 0;
}
