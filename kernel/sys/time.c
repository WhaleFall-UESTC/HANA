#include <common.h>
#include <mm/mm.h>
#include <proc/proc.h>
#include <time.h>
#include <syscall.h>
#include <proc/sched.h>

uint64 tick_counter = 0;

SYSCALL_DEFINE1(times, clock_t, struct tms*, user_buf) {
    struct proc* proc = myproc();

    if (user_buf == NULL)
        return -1;

    KALLOC(struct tms, buf);

    buf->tms_utime = proc->utime;
    buf->tms_stime = proc->stime;
    buf->tms_cutime = proc->utime;
    buf->tms_cstime = proc->stime;

    if (copyout(UPGTBL(proc->pagetable), (uint64) user_buf, buf, sizeof(struct tms)))
        return -1;

    return tick_counter;
}

SYSCALL_DEFINE2(gettimeofday, int, struct timespec*, ts, int*, tz) {
    struct proc* proc = myproc();
    if (ts == NULL)
        return -1;

    KALLOC(struct timespec, ts_buf);
    
    uint64 t = tick_counter * MS_PER_TICK;
    ts_buf->tv_sec = t / 1000UL;
    ts_buf->tv_nsec = (t % 1000UL) * 1000000UL;
    
    if (copyout(UPGTBL(proc->pagetable), (uint64) ts, ts_buf, sizeof(struct timespec)))
        return -1;

    return 0;
}

SYSCALL_DEFINE2(nanosleep, int, struct timespec*, dura, struct timespec*, rem) {
    if (dura->tv_sec < 0 || dura->tv_nsec < 0 || dura->tv_nsec > 999999999)
        return -1;
    struct proc* proc = myproc();
    if (proc == NULL)
        return -1;
    long t_ms = dura->tv_sec * 1000 + dura->tv_nsec / 1000000;
    if (dura->tv_nsec % 1000000 != 0)
        t_ms += 1;
    if (t_ms < 0)
        return -1;
    else if (t_ms == 0)
        return 0;
    proc->state = SLEEPING;
    proc->sleeping_due = tick_counter + (t_ms - 1) / MS_PER_TICK + 1;
    sched();
    return 0;
}