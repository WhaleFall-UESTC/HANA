#ifndef __SCHED_H__
#define __SCHED_H__

void swtch(struct context *old, struct context* nex);
void scheduler();

#endif // __SCHED_H__