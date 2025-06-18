#include <common.h>
#include <arch.h>
#include <debug.h>
#include <mm/mm.h>
#include <mm/memlayout.h>
#include <trap/trap.h>
#include <trap/context.h>
#include <irq/interrupt.h>
#include <proc/proc.h>
#include <proc/sched.h>

extern char kernelvec[], uservec[], trampoline[], userret[];

extern void timer_isr();

handler interrupt_handler_vector[NR_INTERRUPT] = {};
handler exception_handler_vector[NR_EXCEPTION] = {};

static void log_trap_details(int ecode, int esubcode);

void info_exception() {
    Log(ANSI_FG_RED, "ERA: %lx", r_csr_era());
    uint64 estat = r_csr_estat();
    int ecode = ((estat & CSR_ESTAT_Ecode) >> 16);
    int esubcode = ((estat & CSR_ESTAT_EsubCode) >> 22);
    Log(ANSI_FG_RED, "Ecode: %d, Esubcode: %d", ecode, esubcode);
    log_trap_details(ecode, esubcode);
    Log(ANSI_FG_RED, "BADV: %lx", r_csr_badv());
}

__attribute__((aligned(PGSIZE))) void panic_exception() {
    info_exception();
    panic("Exception");
}


void
register_trap_handler(int interrupt, int ecode, void* function)
{
    handler* handler_vector = (interrupt ? interrupt_handler_vector : exception_handler_vector);
    handler_vector[ecode] = (handler) function;
}


void 
trap_init() 
{
    for (int i = 0; i < NR_EXCEPTION; i++)
        exception_handler_vector[i] = NULL;
    for (int i = 0; i < NR_INTERRUPT; i++)
        interrupt_handler_vector[i] = NULL;

    // register handler
    register_trap_handler(INTERRUPT, TI, timer_isr);
    register_trap_handler(INTERRUPT, HWI0, irq_response);
}

void 
trap_init_hart()
{
    // set all exception to the same handler
    // enable timer and hardware interrupt
    w_csr_ecfg(TI_VEC | HWI_VEC);
    w_csr_eentry((uint64)kernelvec);
}

static int 
trap(int ecode) 
{
    handler fn = NULL;
    if (ecode == 0) {
        uint64 estat_is = r_csr_estat();
        uint64 ecfg_lie = r_csr_ecfg();
        // uint64 estat_is = r_csr_estat() & CSR_ESTAT_IS;
        // uint64 ecfg_lie = r_csr_ecfg() & CSR_ECFG_LIE;
        uint64 pending = estat_is & ecfg_lie;
        if (pending) {
            int i;
            for (i = NR_INTERRUPT - 1; i >= 0; i--)
                if (pending >> i)
                    break;
            fn = interrupt_handler_vector[i];
        }
    } else {
        fn = exception_handler_vector[ecode];
    }

    if (fn == NULL)
        return 1;

    fn();
    return 0;
}

void 
kernel_trap() 
{
    int ecode = r_ecode();
    // uint64 badv = r_csr_badv();
    uint64 era = r_csr_era();
    uint64 prmd = r_csr_prmd();

    if (trap(ecode) != 0) {
        info_exception();
        panic("KERNEL TRAP ERROR");
    }

    w_csr_era(era);
    w_csr_prmd(prmd);
}

void
user_trap()
{
    int ecode = r_ecode();

    w_csr_eentry((uint64)kernelvec);

    struct proc* p = myproc();
    p->trapframe->era = r_csr_era();

    if (trap(ecode) != 0) {
        info_exception();
        p->killed = 1;
    }

    if (p->killed)
        exit(-1);

    dive_to_user();
}

void
dive_to_user()
{
    struct proc* p = myproc();
    intr_off();

    w_csr_eentry(TRAMPOLINE + (uservec - trampoline));

    uint64 prmd = r_csr_prmd();
    prmd |= (CSR_PRMD_PPLV | CSR_PRMD_PIE);
    w_csr_prmd(prmd);

    w_csr_era(p->trapframe->era);

    p->trapframe->kernel_sp = p->stack + KSTACK_SIZE;
    p->trapframe->kernel_hartid = r_tp();
    p->trapframe->kernel_trap = (uint64) user_trap;

    uint64 trapframe = TRAPFRAME + ((uint64)p->trapframe - PGROUNDDOWN(p->trapframe));
    uint64 pgdl = (uint64) UPGTBL(p->pagetable);
    
    uint64 fn = TRAMPOLINE + (userret - trampoline);
    ((void (*)(uint64, uint64))fn)(trapframe, pgdl);
}





typedef struct {
    unsigned int ecode;          // 异常码
    unsigned int esubcode;       // 子异常码
    const char* name;            // 异常名称
    const char* description;     // 异常说明
    const char* category;        // 所属类别
} ExceptionInfo;

const ExceptionInfo exceptions[] = {
    {0x1, 0, "PIL", "load操作页无效异常", "地址转换异常"},
    {0x2, 0, "PIS", "store操作页无效异常", "地址转换异常"},
    {0x3, 0, "PIF", "取指操作页无效异常", "地址转换异常"},
    {0x4, 0, "PME", "页修改异常", "地址转换异常"},
    {0x5, 0, "PNR", "页不可读异常", "地址转换异常"},
    {0x6, 0, "PNX", "页不可执行异常", "地址转换异常"},
    {0x7, 0, "PPI", "页权限等级不合规异常", "地址转换异常"},
    {0x8, 0x0, "ADEF", "取指地址错异常", "指令执行中的错误"},
    {0x8, 0x1, "ADEM", "访存指令地址错异常", "指令执行中的错误"},
    {0x9, 0, "ALE", "地址非对齐异常", "指令执行中的错误"},
    {0xA, 0, "BCE", "边界约束检查错异常", "指令执行中的错误"},
    {0xB, 0, "SYS", "系统调用异常", "系统调用和陷入"},
    {0xC, 0, "BRK", "断点异常", "系统调用和陷入"},
    {0xD, 0, "INE", "指令不存在异常", "指令执行中的错误"},
    {0xE, 0, "IPE", "指令权限等级错异常", "指令执行中的错误"},
    {0xF, 0, "FPD", "浮点指令未使能异常", "系统调用和陷入"},
    {0x10, 0, "SXD", "128位向量扩展指令未使能异常", "系统调用和陷入"},
    {0x11, 0, "ASXD", "256位向量扩展指令未使能异常", "系统调用和陷入"},
    {0x12, 0x0, "FPE", "基础浮点指令异常", "需要软件修正的运算"},
    {0x12, 0x1, "VFPE", "向量浮点指令异常", "需要软件修正的运算"},
    {0x13, 0x0, "WPEF", "取指监测点异常", "系统调用和陷入"},
    {0x13, 0x1, "WPEM", "load/store操作监测点异常", "系统调用和陷入"},
};

static void log_trap_details(int ecode, int esubcode) {
    for (int i = ecode - 1; i < 0x13; i++) {
        if (exceptions[i].ecode == ecode && exceptions[i].esubcode == esubcode) {
            Log(ANSI_FG_RED, "Exception: %s (%s)", exceptions[i].name, exceptions[i].description);
            // Log(ANSI_FG_RED, "Category: %s", exceptions[i].category);
            return;
        }
    }
}