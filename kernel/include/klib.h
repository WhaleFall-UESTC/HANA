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

/* Calculation helper functions */
#define MINFUNC_DEFINE(type) \
    static inline type min_##type(const type a, const type b) { \
        return a < b ? a : b; \
    }

#define MAXFUNC_DEFINE(type) \
    static inline type max_##type(const type a, const type b) { \
        return a > b ? a : b; \
    }

MINFUNC_DEFINE(uint8);
MINFUNC_DEFINE(uint16);
MINFUNC_DEFINE(uint32);
MINFUNC_DEFINE(uint64);
MINFUNC_DEFINE(int8);
MINFUNC_DEFINE(int16);
MINFUNC_DEFINE(int32);
MINFUNC_DEFINE(int64);

MAXFUNC_DEFINE(uint8);
MAXFUNC_DEFINE(uint16);
MAXFUNC_DEFINE(uint32);
MAXFUNC_DEFINE(uint64);
MAXFUNC_DEFINE(int8);
MAXFUNC_DEFINE(int16);
MAXFUNC_DEFINE(int32);
MAXFUNC_DEFINE(int64);

#endif // __KLIB_H__
