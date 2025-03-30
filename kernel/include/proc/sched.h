#ifndef __SCHED_H__
#define __SCHED_H__

void swtch(struct context *old, struct context* nex);
void scheduler();
void sched();
void yield();

#endif // __SCHED_H__

