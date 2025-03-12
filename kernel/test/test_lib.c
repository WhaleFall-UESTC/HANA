#include <defs.h>
#include <debug.h>

void
test_printf()
{
    printf("Test print p, show test_printf addr: %p\n", test_printf);
    printf("Test print s, show my name and char 'a': %s %c\n", "WhaleFall", 'a');
    printf("Test print u, show 114514: %u\n", 114514);
    printf("Test print d, show -114514: %d\n", -114514);
    printf("Test print x, show 0x11451a: %#x and without '0x': %X\n", 0x11451A, 0x114514A);
}

static inline void 
test_strcpy() 
{
    char src[] = "Hello World";
    char dest[20];
    
    strcpy(dest, src);
    assert(strcmp(dest, src) == 0);
    
    char short_dest[5];
    strcpy(short_dest, "test");
    assert(strcmp(short_dest, "test") == 0);
}

static inline void 
test_strncpy() 
{
    char src[] = "Hello";
    char dest[6];
    
    strncpy(dest, src, 5);
    dest[5] = '\0';
    assert(strcmp(dest, "Hello") == 0);
    
    char dest2[7];
    strncpy(dest2, src, 7);
    assert(memcmp(dest2, "Hello\0\0", 7) == 0);
}


static inline void test_strncmp() {
    assert(strncmp("apple", "apples", 5) == 0);
    assert(strncmp("apple", "apples", 6) < 0);
    assert(strncmp("zoo", "apple", 1) > 0);
}

static inline void 
test_memset() 
{
    char buf[10];
    memset(buf, 'A', 9);
    buf[9] = '\0';
    assert(strcmp(buf, "AAAAAAAAA") == 0);
    
    memset(buf+2, 'B', 3);
    assert(memcmp(buf, "AABBBAAAA", 9) == 0);
}

static inline void
test_memcpy() 
{
    char src[] = "123456789";
    char dest[10];
    
    memcpy(dest, src, 5);
    assert(memcmp(dest, "12345", 5) == 0);
    
    char nums[10] = {0,1,2,3,4,5,6,7,8,9};
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wrestrict"
    memcpy(nums+2, nums, 3);
    #pragma GCC diagnostic pop
    char nums_after[10] = {0,1,0,1,2,5,6,7,8,9};
    assert(memcmp(nums, nums_after, 10) == 0);
}

static inline void 
test_memmove() 
{
    char buf[10] = "abcdefghi";
    
    memmove(buf+3, buf, 4);
    assert(strcmp(buf, "abcabcdhi") == 0);
    
    char buf2[10] = "abcdefghi";
    memmove(buf2, buf2+2, 4);
    assert(memcmp(buf2, "cdefefghi", 9) == 0);
}

static inline void 
test_memcmp() 
{
    char a[] = {1,2,3,4,5};
    char b[] = {1,2,3,4,5};
    assert(memcmp(a, b, 5) == 0);
    
    char c[] = {1,2,0,4,5};
    assert(memcmp(a, c, 5) > 0);

    assert(memcmp("abcd", "abce", 3) == 0);
}

void 
test_string() 
{
    test_strcpy();
    // puts("Pass strcpy\n");
    test_strncpy();
    // puts("Pass strncpy\n");
    test_strncmp();
    // puts("Pass strncmp\n");
    test_memset();
    // puts("Pass memset\n");
    test_memcpy();
    // puts("Pass memcpy\n");
    test_memmove();
    // puts("Pass memmove\n");
    test_memcmp();
    // puts("Pass memcmp\n");
}
