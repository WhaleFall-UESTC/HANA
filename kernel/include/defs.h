#include <stddef.h>
#include <stdarg.h>
#include <common.h>
#include <riscv.h>

/* devices/uart.c */
void            uart_init(void);
int             uart_putc(char c);


/* lib/string.c */
size_t          strlen(const char *s);
char*           strncpy(char* dst, const char* src, size_t n);
char*           strcpy(char* dst, const char* src);
int             strncmp(const char* s1, const char *s2, size_t n);
int             strcmp(const char *s1, const char *s2);
char*           strcat(char *dst, const char *src);
void*           memset(void *dst, int c, size_t n);
int             memcmp(const void *v1, const void *v2, size_t n);
void*           memmove(void *dst, const void *src, size_t n);
void*           memcpy(void *dst, const void *src, size_t n);

/* lib/printf.c */
int             vsnprintf(char *out, size_t n, const char* fmt, va_list ap);
// int             vsprintf(char* out, const char* fmt, va_list ap);
// int             snprintf(char *out, size_t n, const char *fmt, ...) ;
// int             sprintf(char *out, const char *fmt, ...);
int             printf(const char* fmt, ...);
int             puts(char* buf);


/* mm/buddy.c */
void            buddy_init(uint64 start, uint64 end);
void*           buddy_alloc(uint64 sz);
void            buddy_free(void* addr, int order);


/* mm/slab.c */
void            slab_init();
void*           slab_alloc(uint64 sz);
void            slab_free(void* addr, uint8 nr_free);


/* mm/kalloc.c */
void            kinit();
void*           kalloc(uint64 sz);
void            kfree(void *addr);


/* mm/vm.c */
pte_t*          walk(pagetable_t pgtbl, uint64 va, int alloc);
void            mappages(pagetable_t pgtbl, uint64 va, uint64 pa, uint64 sz, int flag);
pagetable_t     kvmmake();
void            kvminit();
void            kvminithart();


