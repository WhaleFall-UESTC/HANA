#ifndef __DEBUG_H__
#define __DEBUG_H__

#define ANSI_RESET      "\033[0m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_FG_BLACK   "\033[30m"
#define ANSI_FG_RED     "\033[31m"
#define ANSI_FG_GREEN   "\033[32m"
#define ANSI_FG_YELLOW  "\033[33m"
#define ANSI_FG_BLUE    "\033[34m"
#define ANSI_FG_MAGENTA "\033[35m"
#define ANSI_FG_CYAN    "\033[36m"
#define ANSI_FG_WHITE   "\033[37m"
#define ANSI_OVERWRITE_MAGENTA "\033[1A\033[2K\r\033[35m"

int printf(const char *fmt, ...);
int puts(char* buf);

#define Log(color, info, ...) \
    printf(color "[%s:%d %s] " info ANSI_RESET "\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)

// #define Log(color, info, ...) 
//     printf("[%s:%d] " info "\n", 
//         __FILE__, __LINE__, ## __VA_ARGS__)

#define log(info, ...)  Log(ANSI_FG_BLUE, info, ## __VA_ARGS__)
#define error(info, ...) Log(ANSI_FG_RED, info, ## __VA_ARGS__)
#define warn(info, ...) Log(ANSI_FG_YELLOW, info, ##__VA_ARGS__)
#define info(info, ...) Log(ANSI_FG_WHITE, info, ##__VA_ARGS__)

#ifdef DEBUG
#define debug(info, ...) Log(ANSI_FG_MAGENTA, info, ##__VA_ARGS__)
#define debug_dynamic(info, ...) Log(ANSI_OVERWRITE_MAGENTA, info, ##__VA_ARGS__)
#else
#define debug(info, ...)
#endif

#define warn_on(cond, info, ...) \
    do { \
        if (cond) { \
            warn("Warning: " info, ##__VA_ARGS__); \
        } \
    } while (0)

#define Assert(cond, format, ...) \
    do { \
        if (!(cond)) { \
            Log(ANSI_FG_RED, format, ## __VA_ARGS__); \
            for (;;)    \
                ;   \
        } \
    } while (0)

#define assert(cond) Assert(cond, "Assertion failed: " #cond);

#define panic(format, ...) Assert(0, format, ## __VA_ARGS__)

#define TODO() panic("please implement me")

#define PASS(info, ...) Log(ANSI_FG_GREEN, info, ## __VA_ARGS__)
#define out(info, ...)  Log(ANSI_FG_CYAN, info, ## __VA_ARGS__)

#endif // __DEBUG_H__