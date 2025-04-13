#include <loongarch.h>

.section .text._start
.global _start
_start:
    # initialize stack
    la      $sp, init_stack 
    csrrd   $tp, CSR_CPUID
    li.d    $t0, 4096
    addi.d  $a0, $tp, 1
    mul.d   $a0, $a0, $t0
    add.d   $sp, $sp, $a0

    li.d    $t0, CSR_DMW_PLV0 | CSR_DMW_MAT_CC
    csrwr   $t0, CSR_DMW0
    li.d    $t0, 0x0
    csrwr   $t0, CSR_DMW1
    csrwr   $t0, CSR_DMW2
    csrwr   $t0, CSR_DMW3
    csrwr   $t0, CSR_TLBRENTRY
    li.w    $t0, CSR_CRMD_PLV0 & ~CSR_CRMD_IE & ~CSR_CRMD_DA | CSR_CRMD_PG \
                | CSR_CRMD_DATF_CC | CSR_CRMD_DATM_CC
    csrwr   $t0, CSR_CRMD
    invtlb  0x0, $zero, $zero

    bl main

    # if main returns, we should never get here
spin:
    b spin

    
    
