#include <ulib.h>

int sleep(unsigned int seconds) {
    struct timespec req, rem;
    req.tv_sec = seconds;
    req.tv_nsec = 0;

    int result = nanosleep(&req, &rem);

    if (result == 0) {
        return 0; // Sleep completed successfully
    } else {
        // If interrupted, return remaining time
        return rem.tv_sec;
    }
}