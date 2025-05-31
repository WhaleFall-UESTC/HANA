#ifndef __KLIB_H__
#define __KLIB_H__

#include <common.h>
#include <lib/stdarg.h>

/* lib/string.c */
size_t          strlen(const char *s);
char*           strncpy(char* dst, const char* src, size_t n);
char*           strcpy(char* dst, const char* src);
int             strncmp(const char* s1, const char *s2, size_t n);
int             strcmp(const char *s1, const char *s2);
char*           strcat(char *dst, const char *src);
char*           strdup(const char* str);
void*           memset(void *dst, int c, size_t n);
int             memcmp(const void *v1, const void *v2, size_t n);
void*           memmove(void *dst, const void *src, size_t n);
void*           memcpy(void *dst, const void *src, size_t n);

/* lib/printf.c */
int             vsnprintf(char *out, size_t n, const char* fmt, va_list ap);
int             vsprintf(char* out, const char* fmt, va_list ap);
int             snprintf(char *out, size_t n, const char *fmt, ...) ;
int             sprintf(char *out, const char *fmt, ...);
int             printf(const char* fmt, ...);
int             puts(char* buf);

/* lib/sort.c */
typedef int (*compar_fn_t)(const void*, const void*);
/**
 * Sort NMEMB elements of BASE, of SIZE bytes each, using COMPAR to perform the comparisons.
 */
void qsort(void *base, size_t nmemb, size_t size, compar_fn_t compar);

#endif // __KLIB_H__
