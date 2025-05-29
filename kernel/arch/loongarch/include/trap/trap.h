#ifndef __TRAP_H__
#define __TRAP_H__

#include <arch.h>

struct trapframe {
    /*   0 */ uint64 kernel_pgtbl;  // kernel page table
    /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
    /*  16 */ uint64 kernel_trap;   // usertrap() user_trap_handler
    /*  24 */ uint64 era;           // saved user program counter
    /*  32 */ uint64 kernel_hartid; // saved kernel tp 
    /*  40 */ uint64 ra; 
    /*  48 */ uint64 tp; 
    /*  56 */ uint64 sp;            
    /*  64 */ uint64 a0;
    /*  72 */ uint64 a1;
    /*  80 */ uint64 a2;
    /*  88 */ uint64 a3;
    /*  96 */ uint64 a4;
    /* 104 */ uint64 a5;
    /* 112 */ uint64 a6;
    /* 120 */ uint64 a7;
    /* 128 */ uint64 t0;
    /* 136 */ uint64 t1;
    /* 144 */ uint64 t2;
    /* 152 */ uint64 t3;
    /* 160 */ uint64 t4;
    /* 168 */ uint64 t5;
    /* 176 */ uint64 t6;
    /* 184 */ uint64 t7;
    /* 192 */ uint64 t8;
    /* 200 */ uint64 fp;
    /* 208 */ uint64 s0;
    /* 216 */ uint64 s1;
    /* 224 */ uint64 s2;
    /* 232 */ uint64 s3;
    /* 240 */ uint64 s4;
    /* 248 */ uint64 s5;
    /* 256 */ uint64 s6;
    /* 264 */ uint64 s7;
    /* 272 */ uint64 s8;
};

#define trapframe_set_stack(proc, _sp) proc->trapframe->sp = (_sp)
#define trapframe_set_init_func(proc, func) proc->trapframe->ra = (func)
#define trapframe_set_return(proc, ret) proc->trapframe->a0 = (ret)
#define trapframe_set_era(proc, addr) proc->trapframe->era = (addr)
#define context_set_init_func(proc, func) proc->context.ra = (func)
#define context_set_stack(proc, _sp) proc->context.sp = (_sp);

// 地址转换异常
#define PIL 0x1 // load操作页无效异常
#define PIS 0x2 // store操作页无效异常
#define PIF 0x3 // 取指操作页无效异常
#define PME 0x4 // 页修改异常
#define PNR 0x5 // 页不可读异常
#define PNX 0x6 // 页不可执行异常
#define PPI 0x7 // 页权限等级不合规异常

// 指令执行中的错误
#define ADEF 0x8 // 取指地址错异常 (Esubcode: 0x0)
#define ADEF_Esubcode 0x0
#define ADEM 0x8 // 访存指令地址错异常 (Esubcode: 0x1)
#define ADEM_Esubcode 0x1
#define ALE 0x9 // 地址非对齐异常
#define BCE 0xA // 边界约束检查错异常
#define INE 0xD // 指令不存在异常
#define IPE 0xE // 指令权限等级错异常

// 系统调用和陷入
#define SYS 0xB // 系统调用异常
#define BRK 0xC // 断点异常
#define FPD 0xF // 浮点指令未使能异常
#define SXD 0x10 // 128位向量扩展指令未使能异常
#define ASXD 0x11 // 256位向量扩展指令未使能异常
#define WPEF 0x13 // 取指监测点异常 (Esubcode: 0x0)
#define WPEF_Esubcode 0x0
#define WPEM 0x13 // load/store操作监测点异常 (Esubcode: 0x1)
#define WPEM_Esubcode 0x1

// 需要软件修正的运算
#define FPE 0x12 // 基础浮点指令异常 (Esubcode: 0x0)
#define FPE_Esubcode 0x0
#define VFPE 0x12 // 向量浮点指令异常 (Esubcode: 0x1)
#define VFPE_Esubcode 0x1

#define NR_EXCEPTION 0x13

// 中断
#define INT 0x0 

// 机器错误异常
#define MERR 0x0 

// TLB重填异常
#define TLBR 0x0 


// 在LoongArch指令系统中，中断被视作一类特殊的异常进行处理
// 将SWI0～IPI这13个中断依次“视作”异常编号64～76的异常
#define IPI 12
#define TI  11
#define PMI 10
#define HWI7 9
#define HWI6 8
#define HWI5 7
#define HWI4 6
#define HWI3 5
#define HWI2 4
#define HWI1 3
#define HWI0 2
#define SWI1 1
#define SWI0 0

#define TI_VEC (1UL << TI)
#define HWI_VEC (0xff << HWI0)

#define IntEcodeBase 64
#define Int2Ecode(INT) (INT + IntEcodeBase)
#define Ecode2Int(Ecode) (Ecode - IntEcodeBase)

#define NR_INTERRUPT 13


#define INTERRUPT 1
#define EXCEPTION 0

#define TRAP_TYPE(Ecode) (Ecode >= IntEcodeBase ? INTERRUPT : EXCEPTION)


static inline int 
r_ecode() {
    uint64 estat = r_csr_estat();
    int ecode = ((estat & CSR_ESTAT_Ecode) >> 16);
    return ecode;
}

static inline int
r_esubcode() {
    uint64 estat = r_csr_estat();
    int esubcode = ((estat & CSR_ESTAT_EsubCode) >> 22);
    return esubcode;
}


typedef void (*handler)(void);

void    register_trap_handler(int interrupt, int ecode, void* function);
void    trap_init();
void    trap_init_hart();
void    kernel_trap();
void    dive_to_user();

#endif