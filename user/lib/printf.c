
#ifndef __U_ULIB_H__
#define __U_ULIB_H__

#include <stdarg.h>
#include <ulib.h>
#include <syscall.h>

#define ZEROPAD 1   // defalut blank pad
#define BIN     2
#define SIGN    4
#define OCT     8
#define HEX     16
#define LEFT    32  // left align, defalut right
#define SMALL   64
#define PREFIX  128

// if buf too large will make stack overflow
#define BUFMAX 128

#define is_digit(c) ((c) >= '0' && (c) <= '9')
#define fill_left() if (!(flags & LEFT)) { while (field_width-- > 0) { *str++ = ' '; } }
#define fill_right() while (field_width-- > 0) { *str++ = ' '; }

// const char* digits_low = "0123456789abcdefghijklmnopqrstuvwxyz";
// const char* digits_high = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// string convert to number
static inline int
skip_atoi(const char** s)
{
    int i = 0;
    while (is_digit(**s))
        i = i * 10 + *((*s)++) - '0';
    return i;
}


static inline char
digits(unsigned num)
{
    return (char)((num >= 10 ? 87 : 48) + num);
}


// number convert to formatted string
// actually unsupport upper character
static char* 
number(char *str, long num, int size, int precision, int type)
{
    char c = 0, sign = 0, num_str[36];
    int i = 0;
    unsigned base = ((type & BIN) ? BIN : ((type & OCT) ? OCT : ((type & HEX) ? HEX : 10)));
    // const char* digits = (type & SMALL) ? digits_low : digits_high;

    // left align forbid zeropad
    if (type & LEFT) type &= ~ZEROPAD;
    // set padding character
    c = (type & ZEROPAD) ? '0' : ' ';

    if ((type & SIGN) && num < 0)
        sign = '-', num = -num;

    size -= (sign) ? 1 : 0;

    if (type & PREFIX) {
        size -= (((type & BIN) || (type & HEX)) ? 2 : 0);
        size -= ((type & OCT) ? 1 : 0);
    }

    // convert
    if (num == 0)
        num_str[i++] = '0';
    else {
        unsigned long unum = num;
        do {
            num_str[i++] = digits(unum % base);
            unum /= base;
        } while (unum != 0);
    }

    precision = (i > precision ? i : precision);
    size -= precision;

    if (!(type & (ZEROPAD | LEFT))) {
        while (size-- > 0)
            *str++ = ' ';
    }

    if (sign)
        *str++ = sign;

    if (type & PREFIX) {
        *str++ = '0';
        if (type & BIN)
            *str++ = 'b';
        if (type & HEX)
            *str++ = 'x';
    }

    if (!(type & LEFT)) {
        while (size-- > 0)
            *str++ = c;
    }

    while (i < precision--) *str++ = '0';
    while (i-- > 0) *str++ = num_str[i];
    while (size-- > 0) *str++ = ' ';

    return str;
}


int
vsnprintf(char *out, size_t n, const char* fmt, va_list ap)
{
    int cnt = 0;
    char *str;

    for (str = out; *fmt && cnt < n; cnt++, fmt++) 
    {
        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }

        int flags = 0;
        loop:
            cnt++, fmt++;
            switch(*fmt) {
                case '-': flags |= LEFT; goto loop;
                case '#': flags |= PREFIX; goto loop;
                case '0': flags |= ZEROPAD; goto loop;
            }

        int field_width = -1;
        if (is_digit(*fmt)) {
            field_width = skip_atoi(&fmt);
        } else if (*fmt == '*') {
            cnt++, fmt++;
            field_width = va_arg(ap, int);
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        int precision = -1;
        if (*fmt == '.') {
            cnt++, fmt++;
            if (is_digit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*')
                precision = va_arg(ap, int);
            precision = (precision < 0) ? 0 : precision;
        }

        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
            cnt++, fmt++;
        
        switch (*fmt) {
            case 'c':
                fill_left();
                *str++ = (unsigned char) va_arg(ap, int);
                fill_right();
                break;

            case 's':
                char* s = va_arg(ap, char*);
                s = (s) ? s : "(null)";
                int len = strlen(s);
                fill_left();
                for (int i = 0; i < len; i++)
                    *str++ = *s++;
                fill_right();
                break;

            case 'o': 
                flags |= OCT;
                str = number(str, va_arg(ap, unsigned long), field_width, precision, flags);
                break;

            case 'p': 
                if (field_width == -1) {
                    field_width = 8;
                    flags |= ZEROPAD;
                }
                flags |= HEX;
                str = number(str, va_arg(ap, unsigned long), field_width, precision, flags);
                break;

            case 'x': 
                flags |= SMALL;
            case 'X': 
                flags |= HEX;
                str = number(str, va_arg(ap, unsigned long), field_width, precision, flags);
                break;

            case 'd':
                flags |= SIGN;
            case 'u':
                str = number(str, va_arg(ap, unsigned long), field_width, precision, flags);
                break;

            case 'n': 
                char* ip = (char *) va_arg(ap, int *);
                *ip = (str - out);
                break;

            default: 
                if (*fmt != '%')
                    *str++ = '%';
                if (*fmt)
                    *str++ = *fmt;
                else 
                    cnt--, fmt--;
                break;
        }
    }
    *str = '\0';
    return str - out;
}

int
vsprintf(char* out, const char* fmt, va_list ap)
{
    return vsnprintf(out, BUFMAX, fmt, ap);
}

int 
snprintf(char *out, size_t n, const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    int i = vsnprintf(out, n, fmt, args);
    va_end(args);
    return i;
}

int 
sprintf(char *out, const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    int i = vsprintf(out, fmt, args);
    va_end(args);
    return i;
}

int
printf(const char* fmt, ...)
{
    int i;
    char buf[BUFMAX];
    va_list args;
    va_start(args, fmt);
    write(STDOUT_FILENO, buf, i = vsnprintf(buf, BUFMAX, fmt, args));
    va_end(args);
    return i;
}

int 
puts(char* buf)
{
    int i;
    for (i = 0; buf[i] != '\0'; i++)
        putchar(buf[i]);
    return i;
}


#endif // __U_ULIB_H__
