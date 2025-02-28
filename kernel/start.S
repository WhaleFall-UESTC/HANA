# qemu -kernel will load kernel at 0x80000000
# and causes each CPU to jump there

.section .text
.global _start
_start:
    li sp, 0xffffffff
    
    # keep each CPU's hartid in its tp
    csrr a0, mhartid
    mv tp, a0
    
    # set mstatus.MPP to 0x01
    # after mret will enter S mode
    csrr a0, mstatus
    li t0, ~0x1800
    and a0, a0, t0
    li t0, 0x800
    or a0, a0, t0
    csrw mstatus, a0

    # set mepc to main
    la a0, main
    csrw mepc, a0

    # forbid pagetable
    csrw satp, zero

    # delegate all interrupts and exceptions to S mode
    li t0, 0xffff
    csrw medeleg, t0
    csrw mideleg, t0

    # enable interrupts in S mode
    csrr a0, sie 
    ori a0, a0, 0x222 
    csrw sie, a0

    # jal ra, timerinit

    # allow S mode to access all physical memory
    csrwi pmpcfg0, 0x0
    li t0, 0x3fffffffffffffff
    csrw pmpaddr0, t0

    # enter S mode and jump to main
    mret
  
spin:
    j spin


# timerinit:
#     csrr a0, mhartid
    
#     # *(uint64*)CLINT_MTIMECMP(id) = *(uint64*)CLINT_MTIME + interval;
    
#     li t0, 0x200BFF8    # CLINT_MTIME address
#     ld t1, 0(t0)
#     li t2, 1000000      # interval
#     add t1, t1, t2

#     li t3, 0x2004000    # CLINT_MTIMECMP_BASE
#     sll a0, a0, 3
#     add t3, t3, a0
#     sd t1, 0(t3)


#     // prepare information in scratch[] for timervec.
#     // scratch[0..2] : space for timervec to save registers.
#     // scratch[3] : address of CLINT MTIMECMP register.
#     // scratch[4] : desired interval (in cycles) between timer interrupts.
#     la t0, timer_scratch
#     add t0, t0, a0
#     slli a0, a0, 2
#     add t0, t0, a0  # get &timer_scratch[hartid][0]

#     sd t3, 24(t0)   # scratch[3]
#     sd t2, 32(t0)   # scratch[4]

#     csrw mscratch, t0

#     // set the machine-mode trap handler.
#     la t0, timervec
#     csrw mtvec, t0

#     // enable machine-mode interrupts.
#     csrr t0, mstatus
#     ori t0, t0, 0x8     # MIE
#     csrw mstatus, t0

#     // enable machine-mode timer interrupts
#     csrr t0, mie 
#     ori t0, t0, 0x80    # MIE_MTIE
#     csrw mie, t0

#     ret

