#ifndef __TRAP_H__
#define __TRAP_H__

#include <arch.h>

struct trapframe {
    /*   0 */ uint64 kernel_satp;   // kernel page table
    /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
    /*  16 */ uint64 kernel_trap;   // usertrap() user_trap_handler
    /*  24 */ uint64 epc;           // saved user program counter
    /*  32 */ uint64 kernel_hartid; // saved kernel tp 
    /*  40 */ uint64 ra; 
    /*  48 */ uint64 sp; 
    /*  56 */ uint64 gp;            
    /*  64 */ uint64 tp;
    /*  72 */ uint64 t0;
    /*  80 */ uint64 t1;
    /*  88 */ uint64 t2;
    /*  96 */ uint64 s0;
    /* 104 */ uint64 s1;
    /* 112 */ uint64 a0;
    /* 120 */ uint64 a1;
    /* 128 */ uint64 a2;
    /* 136 */ uint64 a3;
    /* 144 */ uint64 a4;
    /* 152 */ uint64 a5;
    /* 160 */ uint64 a6;
    /* 168 */ uint64 a7;
    /* 176 */ uint64 s2;
    /* 184 */ uint64 s3;
    /* 192 */ uint64 s4;
    /* 200 */ uint64 s5;
    /* 208 */ uint64 s6;
    /* 216 */ uint64 s7;
    /* 224 */ uint64 s8;
    /* 232 */ uint64 s9;
    /* 240 */ uint64 s10;
    /* 248 */ uint64 s11;
    /* 256 */ uint64 t3;
    /* 264 */ uint64 t4;
    /* 272 */ uint64 t5;
    /* 280 */ uint64 t6;
};

#define trapframe_set_stack(proc, _sp) proc->trapframe->sp = (_sp)
#define trapframe_set_init_func(proc, func) proc->trapframe->ra = (func)
#define trapframe_set_return(proc, item, ret) proc->trapframe->a0 = (ret)
#define trapframe_set_era(proc, addr) proc->trapframe->epc = (addr)
#define context_set_init_func(proc, func) proc->context.ra = (func)
#define context_set_stack(proc, _sp) proc->context.sp = (_sp);

/* trap/trap.c */

/**
 * trap 初始化，注册异常处理函数
 */
void            trap_init();

/**
 * 设置公共的异常处理入口 kernelvec
 */
void            trap_init_hart();

/**
 * 内核异常处理函数
 */
void            kernel_trap();

/**
 * 注册异常处理函数
 * @param interrupt 类型，是异常还是中断
 * @param code 异常号/中断号
 * @param function 异常处理函数指针
 */
void            register_trap_handler(int interrupt, int code, void* function);

/**
 * 进入用户态的入口
 */
void            dive_to_user();

/**
 * 内核态异常报错
 */
void            kernel_trap_error();

/**
 * 打印异常原因与相关寄存器
 * @param scause 异常原因号
 */
void            log_scause(uint64 scause);



#define INTERRUPT 1
#define EXCEPTION 0

// raise S mode environment interrupt
#define ecall() asm volatile("ecall")

static inline uint64 
trap_get_badv() {
  return r_stval();
}


enum interrupt_irq {
  USER_SOFTWARE_INTERRUPT,
  SUPERVISOR_SOFTWARE_INTERRUPT,
  RESERVED_INTERRUPT_1,
  MACHINE_SOFTWARE_INTERRUPT,

  USER_TIMER_INTERRUPT,
  SUPERVISOR_TIMER_INTERRUPT,
  RESERVED_INTERRUPT_2,
  MACHINE_TIMER_INTERRUPT,

  USER_EXTERNAL_INTERRUPT,
  SUPERVISOR_EXTERNEL_INTERRUPT,
  RESERVED_INTERRUPT_3,
  MACHINE_EXTERNAL_INTERRUPT,

  RESERVED_INTERRUPT_4,

  NR_INTERRUPT
};

enum exception_code {
  INTERRUPT_ADDRESS_MISALIGNED,
  INSTRUCTION_ACCESS_FAULT,
  ILLEGAL_INSTRUCTIONS,
  BREAKPOINT,
  LOAD_ADDRESS_MISSALIGNED,
  LOAD_ACCESS_FAULT,
  STORE_AMO_ADDRESS_MISSALIGNED,
  STORE_AMO_ACCESS_FAULT,
  ENVIRONMENT_CALL_FROM_U_MODE,
  ENVIRONMENT_CALL_FROM_S_MODE,
  RESERVED_EXCEPTION_1,
  ENVIRONMENT_CALL_FROM_M_MODE,
  INSTRUCTION_PAGE_FAULT,
  LOAD_PAGE_FAULT,
  RESERVED_EXCEPTION_2,
  STORE_AMO_PAGE_FAULT,
  RESERVED_EXCEPTION_3,

  NR_EXCEPTION
};

#endif // __TRAP_H__