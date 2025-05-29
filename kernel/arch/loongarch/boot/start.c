#include <common.h>
#include <arch.h>
#include <mm/memlayout.h>
#include <debug.h>

extern int main();
extern void timer_init();

void start() {
    /* set direct mapping windows */
    /* in PLV0, map 0x9000_0000_0000_0000 - 0x9000_FFFF_FFFF_FFFF to
           0x0 - 0xFFFF_FFFF_FFFF 
       MAT = CC (Coherent Cache)
       if virtual address is not in the range, it will be translated by page table
    */
    w_csr_dmw0(DMW_MASK | CSR_DMW_PLV0 | CSR_DMW_MAT_CC);
    w_csr_dmw1(0);
    w_csr_dmw2(0);
    w_csr_dmw3(0);

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
    w_csr_crmd(crmd);

    invtlb();

    timer_init();

    main();

    panic("_start should never return");
}

