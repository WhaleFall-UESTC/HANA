#include <defs.h>
#include <debug.h>

void
test_printf()
{
    printf("Test print d, show -114514:\t%d\n", -114514);
    printf("Test print x, show 0x11451a:\t%#x and upper: %#X\n", 0x11451A, 0x114514A);
    printf("Test print p, show main:\t%p\n", test_printf);
    printf("Test print u, show 114514:\t%u\n", 114514);
}