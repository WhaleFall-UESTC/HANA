#define ANSI_RESET "\x1b[0m"
#define ANSI_BOLD "\x1b[1m"
#define ANSI_FG_BLACK "\x1b[30m"
#define ANSI_FG_RED "\x1b[31m"
#define ANSI_FG_GREEN "\x1b[32m"
#define ANSI_FG_YELLOW "\x1b[33m"
#define ANSI_FG_BLUE "\x1b[34m"
#define ANSI_FG_MAGENTA "\x1b[35m"
#define ANSI_FG_CYAN "\x1b[36m"
#define ANSI_FG_WHITE "\x1b[37m"


#define Log(color, info, ...) \
    printf(color "[%s:%d] " info ANSI_RESET "\n", \
        __FILE__, __LINE__, ## __VA_ARGS__)

#define log(info, ...)  Log(ANSI_FG_BLUE, info, ## __VA_ARGS__)

#define Assert(cond, format, ...) \
    do { \
        if (!(cond)) { \
            Log(ANSI_FG_RED, format, ## __VA_ARGS__); \
        } \
    } while (0)

#define assert(cond) Assert(cond, "Assertion failed: " #cond);

#define panic(format, ...) Assert(0, format, ## __VA_ARGS__)

#define TODO() panic("please implement me")

#define PASS(info, ...) Log(ANSI_FG_GREEN, info, ## __VA_ARGS__)
#define out(info, ...)  Log(ANSI_FG_CYAN, info, ## __VA_ARGS__)