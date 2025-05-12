#include <common.h>
#include <loongarch.h>
#include <debug.h>
#include <mm/mm.h>
#include <trap/trap.h>

extern char kernelvec[];

extern void timer_isr();

handler interrupt_handler_vector[NR_INTERRUPT] = {};
handler exception_handler_vector[NR_EXCEPTION] = {};


void info_exception() {
    Log(ANSI_FG_RED, "ERA: %lx", r_csr_era());
    uint64 estat = r_csr_estat();
    int ecode = ((estat & CSR_ESTAT_Ecode) >> 16);
    int esubcode = ((estat & CSR_ESTAT_EsubCode) >> 22);
    Log(ANSI_FG_RED, "Ecode: %d, Esubcode: %d", ecode, esubcode);
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
        uint64 estat_is = r_csr_estat() & CSR_ESTAT_IS;
        uint64 ecfg_lie = r_csr_ecfg() & CSR_ECFG_LIE;
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
        panic("KERNEL TRAP ERROR");
    }

    w_csr_era(era);
    w_csr_prmd(prmd);
}
