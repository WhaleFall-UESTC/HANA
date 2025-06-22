#include <common.h>
#include <mm/mm.h>
#include <proc/proc.h>
#include <time.h>
#include <syscall.h>

uint64 tick_counter = 0;

SYSCALL_DEFINE1(times, clock_t, struct tms*, user_buf) {
    struct proc* proc = myproc();
    KALLOC(struct tms, buf);

    buf->tms_utime = proc->utime;
    buf->tms_stime = proc->stime;
    buf->tms_cutime = proc->utime;
    buf->tms_cstime = proc->stime;

    if (copyout(UPGTBL(proc->pagetable), (uint64) user_buf, buf, sizeof(struct tms)))
        return -1;

    return tick_counter * INTERVAL;
}