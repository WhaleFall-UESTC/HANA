# qemu -kernel will load kernel at 0x80000000
# and causes each CPU to jump there
.section .text._start
.global _start
_start:
#ifdef BIOS_SBI
    addi tp, a0, 0

    # set initial stack
    la sp, init_stack
    li t0, 8192
    mul a0, tp, t0
    add a0, a0, t0
    add sp, sp, a0

    call timer_init

    j main

#else
    # keep each CPU's hartid in its tp
    csrr tp, mhartid

    # set initial stack
    la sp, init_stack
    li t0, 4096
    mul a0, tp, t0
    add a0, a0, t0
    add sp, sp, a0
    
    # set mstatus.MPP to 0x01
    # after mret will enter S mode
    li a0, 0x1000
    csrrc x0, mstatus, a0
    srli a0, a0, 1
    csrrs x0, mstatus, a0

    # set mepc to main
    la a0, main
    csrw mepc, a0

    # forbid pagetable
    csrw satp, x0

    # delegate all interrupts and exceptions to S mode
    li t0, 0xffff
    csrw medeleg, t0
    csrw mideleg, t0

    # enable interrupts in S mode
    li a0, 0x222    # SIE_SEIE | SIE_STIE | SIE_SSIE
    csrrs x0, sie, a0

    # allow S mode to access all physical memory
    csrwi pmpcfg0, 0xf
    li t0, 0x3fffffffffffff
    csrw pmpaddr0, t0

    call timer_init

    # enter S mode and jump to main
    mret
#endif
