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

void kernel_trap_error() {
    info_exception();
    panic("KERNEL TRAP ERROR");
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

    register_trap_handler(EXCEPTION, PME, store_page_fault_handler);
    register_trap_handler(EXCEPTION, PIL, page_unmap_handler);
    register_trap_handler(EXCEPTION, PIS, page_unmap_handler);
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
        kernel_trap_error();
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
        do_exit(-1);

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
} ExceptionInfo;

const ExceptionInfo exceptions[] = {
    {0x1, 0, "PIL", "Page invalid exception for load"},
    {0x2, 0, "PIS", "Page invalid exception for store"},
    {0x3, 0, "PIF", "Page invalid exception for fetch"},
    {0x4, 0, "PME", "Page modified exception"},
    {0x5, 0, "PNR", "Page Not Readable exception"},
    {0x6, 0, "PNX", "Page Not Executable exception"},
    {0x7, 0, "PPI", "Page Privilege error"},
    {0x8, 0x0, "ADEF", "Address error for instruction fetch"},
    {0x8, 0x1, "ADEM", "Address error for Memory access"},
    {0x9, 0, "ALE", "Address Alignment Exception"},
    {0xA, 0, "BCE", "Bound Check Exception"},
    {0xB, 0, "SYS", "Syscall"},
    {0xC, 0, "BRK", "Break"},
    {0xD, 0, "INE", "Instruction Non-Existent"},
    {0xE, 0, "IPE", "Instruction privilege error"},
    {0xF, 0, "FPD", "Floating Point Disabled"},
    {0x10, 0, "SXD", "128 bit vector instructions Disable exception"},
    {0x11, 0, "ASXD", "256 bit vector instructions Disable exception"},
    {0x12, 0x0, "FPE", "Floating Point Exception"},
    {0x12, 0x1, "VFPE", "Vector Floating-Point Exception"},
    {0x13, 0x0, "WPEF", "Instruction Fetch Watchpoint Exception"},
    {0x13, 0x1, "WPEM", "Load/Store Operation Watchpoint Exception"},
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