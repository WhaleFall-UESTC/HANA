#define ANSI_RESET   "\033[0m"
#define ANSI_FG_BLUE "\033[34m"
#define ANSI_FG_RED  "\033[31m"

#define ANSI_FMT(str, fmt) fmt str ANSI_RESET

#define Log(color, format, ...) \
    printf(ANSI_FMT("[%s:%d %s] " format, color) "\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define log(info, ...) \
  printf(ANSI_FMT("[%s:%d %s] " info, ANSI_FG_RED) "\n", \
    __FILE__, __LINE__, __func__, ## __VA_ARGS__)
    

#define Assert(cond, format, ...) \
  do { \
    if (!(cond)) { \
      log(format, ## __VA_ARGS__); \
    } \
  } while (0)

#define assert(cond) \
  do { \
    if (!(cond)) { \
      log("Assertion failed"); \
    } \
  } while (0)

#define panic(format, ...) assert(0, format, ## __VA_ARGS__)

#define TODO() panic("please implement me")