#include <common.h>
#include <proc/proc.h>
#include <time.h>
#include <syscall.h>

uint64 tick_counter = 0;

SYSCALL_DEFINE1(times, clock_t, struct tms*, buf) {
    struct proc* proc = myproc();
    
    buf->tms_utime = proc->utime;
    buf->tms_stime = proc->stime;
    buf->tms_cutime = proc->utime;
    buf->tms_cstime = proc->stime;

    return tick_counter * INTERVAL;
}