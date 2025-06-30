#ifndef __TIME_H__
#define __TIME_H__

#include <common.h>
#include <proc/proc.h>

#define TICK_HZ 100

#ifdef __loongarch64
#define CLOCK_FREQUNCY  100000000UL
#else
#define CLOCK_FREQUNCY  10000000UL
#endif

#define INTERVAL (CLOCK_FREQUNCY / TICK_HZ)
#define MS_PER_TICK (1000 / TICK_HZ)

#define clock_t uint64

struct tms {
	clock_t tms_utime;
	clock_t tms_stime;
	clock_t tms_cutime;
	clock_t tms_cstime;
};

struct timespec {
	time_t tv_sec;        /* second */
	long   tv_nsec;       /* nanosecond */
};

extern uint64 tick_counter;

#endif