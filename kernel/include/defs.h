#include <stddef.h>
#include <stdarg.h>

/* devices/uart.c */
void            uart_init(void);
int             uart_putc(char c);


/* lib/string.c */
size_t          strlen(const char *s);
char*           strncpy(char* dst, const char* src, size_t n);
char*           strcpy(char* dst, const char* src);
int             strncmp(const char* s1, const char *s2, size_t n);
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


