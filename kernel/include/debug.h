#define log(format, ...) \
    printf(ANSI_FMT("[%s:%d %s] " format, ANSI_FG_BLUE) "\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define assert(cond, format, ...) \
  do { \
    if (!(cond)) { \
      log(format, ## __VA_ARGS__); \
    } \
  } while (0)

#define panic(format, ...) assert(0, format, ## __VA_ARGS__)

#define TODO() panic("please implement me")