#include <common.h>
#include <mm/memlayout.h>
#include <debug.h>
#include <trap.h>
#include <irq/interrupt.h>

#ifdef ARCH_RISCV
#include <riscv.h>
#endif

#include <context.h>
#include <proc/proc.h>

extern char kernelvec[], trampoline[], uservec[], userret[];
void timer_interrupt_handler();

typedef void (*handler)(void);

handler interrupt_handler_vector[NR_INTERRUPT] = {};
handler exception_handler_vector[NR_EXCEPTION] = {};

#define TRAP_PANIC(reason, ...) do { \
    Log(ANSI_FG_RED, "scause: %p", (void*)scause); \
    Log(ANSI_FG_RED, "sepc: %p", (void*)sepc); \
    Log(ANSI_FG_RED, "sstatus: %p", (void*)sstatus); \
    panic("kernel_trap: %s code", reason, ## __VA_ARGS__); \
} while(0)


static void
trap()
{
    uint64 scause = r_scause();
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();

    char unexpected[] = "unexpected";
    char unregistered[] = "unregistered";
    char* reason = unexpected;

    uint64 is_interrupt = (scause & (1L << 63));
    handler* handler_vector = (is_interrupt ? interrupt_handler_vector : exception_handler_vector);
    int code = scause & 0xff;
    if (code >= (is_interrupt ? NR_INTERRUPT : NR_EXCEPTION)) 
        goto error_trap;
    handler fn = handler_vector[code];
    if (fn == NULL) {
        reason = unregistered;
        goto error_trap;
    }
    fn();

    return;

error_trap:
    TRAP_PANIC(reason);
}


void
kernel_trap()
{
    log("kernel trap");
    trap();
}


static void
s_mode_ecall_handler()
{
    log("Raise S mode ecall");
    w_sepc(r_sepc() + 4);
}


void 
trap_init()
{
    for (int i = 0; i < NR_INTERRUPT; i++)
        interrupt_handler_vector[i] = NULL;
    for (int i = 0; i < NR_EXCEPTION; i++)
        exception_handler_vector[i] = NULL;

    register_trap_handler(INTERRUPT, SUPERVISOR_SOFTWARE_INTERRUPT, timer_interrupt_handler);
    register_trap_handler(EXCEPTION, ENVIRONMENT_CALL_FROM_S_MODE, s_mode_ecall_handler);
    register_trap_handler(INTERRUPT, MACHINE_EXTERNAL_INTERRUPT, response_interrupt);
}


void
trap_init_hart()
{
    w_stvec((uint64)kernelvec);
    log("sstatus: %p", (void*)r_sstatus());
    log("sie: %p", (void*)r_sie());
    log("stvec: %p", (void*)r_stvec());
}


void
register_trap_handler(int interrupt, int code, void* function)
{
    handler* handler_vector = (interrupt ? interrupt_handler_vector : exception_handler_vector);
    handler_vector[code] = (handler) function;
}


void
usertrap()
{
    log("user trap");
    trap();
}


void
dive_to_user()
{
    struct proc* p = myproc();

    intr_off();

    // w_stvec();
    w_stvec(TRAMPOLINE  + (uservec - trampoline));

    uint64 sstatus = r_sstatus();
    // set SPP to user and enable interrupt in U mode
    w_sstatus((sstatus & ~SSTATUS_SPP) | SSTATUS_SPIE);

    w_sepc(p->trapframe->epc);

    p->trapframe->kernel_sp = p->stack + KSTACK_SIZE;
    p->trapframe->kernel_satp = r_satp();
    p->trapframe->kernel_hartid = r_tp();
    p->trapframe->kernel_trap = (uint64) usertrap;
    
    uint64 satp = MAKE_SATP(p->pagetable);

    uint64 fn = TRAMPOLINE + (userret - trampoline);
    ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}
