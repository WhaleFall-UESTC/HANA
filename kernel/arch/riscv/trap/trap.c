#include <common.h>
#include <mm/memlayout.h>
#include <mm/mm.h>
#include <debug.h>
#include <irq/interrupt.h>
#include <arch.h>
#include <trap/trap.h>
#include <trap/context.h>
#include <proc/proc.h>
#include <syscall.h>

extern char kernelvec[], trampoline[], uservec[], userret[];
void timer_interrupt_handler();
static void syscall_handler();

typedef void (*handler)(void);

handler interrupt_handler_vector[NR_INTERRUPT] = {};
handler exception_handler_vector[NR_EXCEPTION] = {};

#define GET_S_REGS() \
uint64 sstatus = r_sstatus(); \
uint64 sepc = r_sepc(); \
uint64 scause = r_scause();

#define LOG_ERROR_TRAP() do { \
    log_scause(scause); \
    Log(ANSI_FG_RED, "sepc: %p,\tsstatus: %p,\tstval: %p", (void*)sepc, (void*)sstatus, (void*)r_stval()); \
} while(0)

void kernel_trap_error() {
    GET_S_REGS();
    LOG_ERROR_TRAP();
    panic("KERNEL TRAP ERROR");
}


// choose handler by scause
// return 0 if find handler
// return -1 if errors occur
// return 1 if handler not registered
static int
trap(uint64 scause)
{
    uint64 is_interrupt = (scause & (1L << 63));
    handler* handler_vector = (is_interrupt ? interrupt_handler_vector : exception_handler_vector);
    int code = scause & 0xff;
    if (code >= (is_interrupt ? NR_INTERRUPT : NR_EXCEPTION)) 
        return -1;
    handler fn = handler_vector[code];
    if (fn == NULL) 
        return 1;
    fn();

    return 0;
}


void
kernel_trap()
{
    // log("catch kernel trap");
    GET_S_REGS();

    int res = trap(scause);
    if (res != 0) {
        LOG_ERROR_TRAP();
        panic("KERNEL TRAP ERROR");
    }

    // yield() may have caused come trap to occur,
    // so, restore trap registers for use
    w_sepc(sepc);
    w_sstatus(sstatus);
}


static void
s_mode_ecall_handler()
{
    debug("Raise S mode ecall");
    w_sepc(r_sepc() + 4);
}


void 
trap_init()
{
    for (int i = 0; i < NR_INTERRUPT; i++)
        interrupt_handler_vector[i] = NULL;
    for (int i = 0; i < NR_EXCEPTION; i++)
        exception_handler_vector[i] = NULL;

    #ifdef BIOS_SBI
    register_trap_handler(INTERRUPT, SUPERVISOR_TIMER_INTERRUPT, timer_interrupt_handler);
    #else
    register_trap_handler(INTERRUPT, SUPERVISOR_SOFTWARE_INTERRUPT, timer_interrupt_handler);
    register_trap_handler(EXCEPTION, ENVIRONMENT_CALL_FROM_S_MODE, s_mode_ecall_handler);
    #endif
    register_trap_handler(EXCEPTION, ENVIRONMENT_CALL_FROM_U_MODE, syscall_handler);
    register_trap_handler(INTERRUPT, SUPERVISOR_EXTERNEL_INTERRUPT, irq_response);

    register_trap_handler(EXCEPTION, STORE_AMO_PAGE_FAULT, store_page_fault_handler);
    register_trap_handler(EXCEPTION, LOAD_ACCESS_FAULT, page_unmap_handler);
    register_trap_handler(EXCEPTION, STORE_AMO_ACCESS_FAULT, page_unmap_handler);
}


void
trap_init_hart()
{
    w_stvec((uint64)kernelvec);
    debug("sstatus: %p", (void*)r_sstatus());
    debug("sie: %p", (void*)r_sie());
    debug("stvec: %p", (void*)r_stvec());
}


void
register_trap_handler(int interrupt, int code, void* function)
{
    handler* handler_vector = (interrupt ? interrupt_handler_vector : exception_handler_vector);
    handler_vector[code] = (handler) function;
}


void
user_trap()
{
    // log("catch user trap");
    GET_S_REGS();
    
    w_stvec((uint64) kernelvec);

    struct proc* p = myproc();
    p->trapframe->epc = r_sepc();

    int res = trap(scause);
    if (res != 0) {
        LOG_ERROR_TRAP();
        // error occurs, kill this proc
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

    // log("dive into user mode");

    // w_stvec();
    w_stvec(TRAMPOLINE  + (uservec - trampoline));

    uint64 sstatus = r_sstatus();
    // set SPP to user and enable interrupt in U mode
    w_sstatus((sstatus & ~SSTATUS_SPP) | SSTATUS_SPIE);

    w_sepc(p->trapframe->epc);

    p->trapframe->kernel_sp = p->stack + KSTACK_SIZE;
    p->trapframe->kernel_satp = r_satp();
    p->trapframe->kernel_hartid = r_cpuid();
    p->trapframe->kernel_trap = (uint64) user_trap;
    p->trapframe->tp = p->tls_base;
    
    uint64 trapframe = TRAPFRAME + (uint64)p->trapframe - PGROUNDDOWN(p->trapframe);
    uint64 satp = MAKE_SATP(UPGTBL(p->pagetable));

    uint64 fn = TRAMPOLINE + (userret - trampoline);
    ((void (*)(uint64,uint64))fn)(trapframe, satp);
}


static void 
syscall_handler()
{
    struct proc *p = myproc();
    if (p->killed)
        do_exit(-1);
    
    intr_on();

    syscall();
}




#define CASE_CAUSE(code) \
case code: \
    Log(ANSI_FG_RED, "unregistered scause: %s %p",  #code, (void*)scause); \
    break


void __attribute__((unused))
log_scause(uint64 scause)
{
    uint64 code = scause;
    if (scause & (1L << 63)) { // 中断处理
        code &= 0xff;
        switch (code) {
            CASE_CAUSE(USER_SOFTWARE_INTERRUPT);
            CASE_CAUSE(SUPERVISOR_SOFTWARE_INTERRUPT);
            CASE_CAUSE(RESERVED_INTERRUPT_1);
            CASE_CAUSE(MACHINE_SOFTWARE_INTERRUPT);
            CASE_CAUSE(USER_TIMER_INTERRUPT);
            CASE_CAUSE(SUPERVISOR_TIMER_INTERRUPT);
            CASE_CAUSE(RESERVED_INTERRUPT_2);
            CASE_CAUSE(MACHINE_TIMER_INTERRUPT);
            CASE_CAUSE(USER_EXTERNAL_INTERRUPT);
            CASE_CAUSE(SUPERVISOR_EXTERNEL_INTERRUPT);
            CASE_CAUSE(RESERVED_INTERRUPT_3);
            CASE_CAUSE(MACHINE_EXTERNAL_INTERRUPT);
            CASE_CAUSE(RESERVED_INTERRUPT_4);
            default:
                goto unknown_trap;
        }
    }
    else { 
        switch (code) {
            CASE_CAUSE(INTERRUPT_ADDRESS_MISALIGNED);
            CASE_CAUSE(INSTRUCTION_ACCESS_FAULT);
            CASE_CAUSE(ILLEGAL_INSTRUCTIONS);
            CASE_CAUSE(BREAKPOINT);
            CASE_CAUSE(LOAD_ADDRESS_MISSALIGNED);
            CASE_CAUSE(LOAD_ACCESS_FAULT);
            CASE_CAUSE(STORE_AMO_ADDRESS_MISSALIGNED);
            CASE_CAUSE(STORE_AMO_ACCESS_FAULT);
            CASE_CAUSE(ENVIRONMENT_CALL_FROM_U_MODE);
            CASE_CAUSE(ENVIRONMENT_CALL_FROM_S_MODE);
            CASE_CAUSE(RESERVED_EXCEPTION_1);
            CASE_CAUSE(ENVIRONMENT_CALL_FROM_M_MODE);
            CASE_CAUSE(INSTRUCTION_PAGE_FAULT);
            CASE_CAUSE(LOAD_PAGE_FAULT);
            CASE_CAUSE(RESERVED_EXCEPTION_2);
            CASE_CAUSE(STORE_AMO_PAGE_FAULT);
            CASE_CAUSE(RESERVED_EXCEPTION_3);
            default:
                goto unknown_trap;
        }
    }

    return;

unknown_trap:
    Log(ANSI_FG_RED, "unknown scause: %p", (void*)scause); 
}
