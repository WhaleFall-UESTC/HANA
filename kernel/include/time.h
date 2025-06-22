#ifndef __TIME_H__
#define __TIME_H__

#include <common.h>
#include <proc/proc.h>

#define INTERVAL 0x1000000UL

#define clock_t uint64

struct tms {
	clock_t tms_utime;
	clock_t tms_stime;
	clock_t tms_cutime;
	clock_t tms_cstime;
};

extern uint64 tick_counter;

#endif