#include <common.h>
#include <mm/mm.h>
#include <debug.h>

size_t 
strlen(const char *s)
{
    int i;
    for (i = 0; s[i] != '\0'; i++)
        ;
    return i;
}


char* 
strncpy(char* dst, const char* src, size_t n)
{
    char* origin_dst = dst;
    while (n-- > 0 && (*dst++ = *src++) != '\0')
        ;
    *dst++ = '\0';
    return origin_dst;
}


// require len(dst) <= len(src)
char*
strcpy(char* dst, const char* src)
{
    char* origin_dst = dst;
    while (*src)
        *dst++ = *src++;
    *dst = '\0';
    return origin_dst;
}


int
strncmp(const char* s1, const char *s2, size_t n)
{
    while (n > 0 && *s1 && *s1 == *s2)
        n--, s1++, s2++;
    if (n == 0)
        return 0;
    return *s1 - *s2;
}

char*
strcat(char *dst, const char *src) 
{
    int len = strlen(dst), i;
    for (i = 0; src[i]!= '\0'; i++)
        dst[i + len] = src[i];
  
    dst[i + len] = '\0';
    return dst;
}
  

int 
strcmp(const char *s1, const char *s2) 
{
    int i;
    for (i = 0; s1[i] != '\0'; i++)
        if (s1[i] != s2[i])
            return s1[i] - s2[i];
    
    return (s2[i] == '\0') ? 0 : -s2[i];
}


char*
strdup(const char* str)
{
    int len = strlen(str);
    char* dst = kalloc(len);
    return strncpy(dst, str, len);
}


void*
memset(void *dst, int c, size_t n)
{
    char *cdst = (char *) dst;
        int i;
    for (i = 0; i < n; i++) {
        cdst[i] = c;
    }
    return dst;
}


int
memcmp(const void *v1, const void *v2, size_t n)
{
    const char *s1, *s2;

    s1 = v1;
    s2 = v2;
    while(n-- > 0){
        if(*s1 != *s2)
            return *s1 - *s2;
        s1++, s2++;
    }

    return 0;
}


void*
memmove(void *dst, const void *src, size_t n)
{
    const char *s;
    char *d;

    if(n == 0)
        return dst;
  
    s = src;
    d = dst;
    if(s < d && s + n > d){
        s += n;
        d += n;
        while(n-- > 0)
            *--d = *--s;
    } else
        while(n-- > 0)
            *d++ = *s++;

    return dst;
}


void*
memcpy(void *dst, const void *src, size_t n)
{
    return memmove(dst, src, n);
}

void* memdup(const void *src, size_t n)
{
    void *dst = kalloc(n);
    return memcpy(dst, src, n);
}


void name_append_suffix(char *name, int buflen, const char *suffix) {
    int len = strlen(name), suflen = strlen(suffix);
    int pos;

    if(len + suflen < buflen) {
        pos = len;
    }
    else if(buflen < len) {
        pos = buflen - suflen - 1;
    }
    else {
        error("device name buffer too small for suffix");
        return;
    }

    assert(pos >= 0 && pos + suflen < buflen);
    strncpy(name + pos, suffix, suflen);
    name[pos + suflen] = '\0';
}