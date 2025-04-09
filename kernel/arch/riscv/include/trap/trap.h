
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


/* trap/trap.c */
void            trap_init();
void            trap_init_hart();
void            kernel_trap();
void            register_trap_handler(int interrupt, int code, void* function);
void            dive_to_user();

void            syscall_handler();



#define INTERRUPT 1
#define EXCEPTION 0

// raise S mode environment interrupt
#define ecall() asm volatile("ecall")


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


#define CASE_CAUSE(code) \
case code: \
    Log(ANSI_FG_RED, "unregistered scause: %s %p",  #code, (void*)scause); \
    break


static void __attribute__((unused))
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
    else { // 异常处理
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