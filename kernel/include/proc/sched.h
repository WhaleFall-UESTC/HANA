#ifndef __SCHED_H__
#define __SCHED_H__

/**
 * 切换内核态上下文，将目前的 ra, sp, 以及保存寄存器存入 old，将 nex 中的上下文加载出来
 * 在调用者的视角来看就像是调用了一个函数一样，实则已经完成了一次切换进程再切换回来的操作
 * @param old 当前上下文存入的地方
 * @param nex 存放将要加载的上下文
 */
void swtch(struct context *old, struct context* nex);

/**
 * 调度器，负责完成选择一个进程，并将其调度的过程
 * 每一个核都会在最开始的时候运行一个调度器，并且再死循环中不断重复这个过程
 * 这个“过程”被抽象为一个调度器进程
 */
void scheduler();

/**
 * 保存当前的中断状态，并返回到 scheduler 进程
 */
void sched();

/** 
 * 主动让出 CPU，调用 sched 返回 scheduler
 */
void yield();

#endif // __SCHED_H__

