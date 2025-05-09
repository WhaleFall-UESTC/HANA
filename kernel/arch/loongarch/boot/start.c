#include <common.h>
#include <loongarch.h>
#include <mm/memlayout.h>
#include <debug.h>

extern char init_stack[];
extern int main();
extern void tlb_refill();

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

// __attribute__((aligned(PGSIZE))) void panic_tlb_refill_exception() {
//     Log(ANSI_FG_RED, "TLBRERA: %lx", r_csr_tlbrera());
//     Log(ANSI_FG_RED, "TLBRBAV: %lx", r_csr_tlbrbadv());
//     Log(ANSI_FG_RED, "TLBRPRMD: %lx", r_csr_tlbrprmd());
//     Log(ANSI_FG_RED, "CRMD: %lx", r_csr_crmd());
//     panic("TLB refill exception");
// }

void start() {
    // set each core stack
    // uint64 cpuid = r_csr_cpuid();
    // w_tp(cpuid);
    // uint64 stack_pointer = (uint64) init_stack;
    // stack_pointer += (cpuid + 1) * KSTACK_SIZE;
    // w_sp(stack_pointer);

    /* set direct mapping windows */
    /* in PLV0, map 0x9000_0000_0000_0000 - 0x9000_FFFF_FFFF_FFFF to
           0x0 - 0xFFFF_FFFF_FFFF 
       MAT = CC (Coherent Cache)
       if virtual address is not in the range, it will be translated by page table
    */
    csr_write(CSR_DMW0, DMW_MASK | CSR_DMW_PLV0 | CSR_DMW_MAT_CC);
    csr_write(CSR_DMW1, 0);
    csr_write(CSR_DMW2, 0);
    csr_write(CSR_DMW3, 0);

    csr_write(CSR_TLBRENTRY, 0);

    // current mode PLV0 & disable global interrupt
    uint64 crmd = (CSR_CRMD_PLV0 & ~CSR_CRMD_IE);
    // Enable address mapping
    crmd |= CSR_CRMD_PG;
    // when MMU in direct translation mode, 
    // the instruction access type depends on DATF, load/store access type depends on DATM
    // which is set to CC (Coherent Cache).
    // In mapping mode, if instruction/load/store address is in one of Direct Mapping Windows,
    // the access type depends on DMW MAT
    // else, it depends on page table entry MAT
    crmd |= (CSR_CRMD_DATF_CC | CSR_CRMD_DATM_CC);
    csr_write(CSR_CRMD, crmd);

    invtlb();

    w_csr_ecfg(0);
    w_csr_eentry((uint64)panic_exception);
    w_csr_tlbrentry((uint64)tlb_refill);   

    main();

    panic("_start should never return");
}

