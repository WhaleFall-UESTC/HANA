#ifndef __LIB_H__
#define __LIB_H__

#include <stdarg.h>
#include "syscall.h"

typedef unsigned long size_t;

#define null (void*)0

/* lib/string.c */
int             atoi(const char *s);
size_t          strlen(const char *s);
char*           strncpy(char* dst, const char* src, size_t n);
char*           strcpy(char* dst, const char* src);
int             strncmp(const char* s1, const char *s2, size_t n);
int             strcmp(const char *s1, const char *s2);
char*           strcat(char *dst, const char *src);
char*           strchr(const char *s, int c);
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
// int             fprintf(int fd, const char *fmt, ...);
int             puts(char *buf);
void*           malloc(long unsigned int nbytes);
void            free(void *);

/* lib/sleep.c */
struct timespec {
    time_t tv_sec; /* seconds */
    long tv_nsec;  /* and nanoseconds */
};
int sleep(unsigned int seconds);

#endif // __LIB_H__