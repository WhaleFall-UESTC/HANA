#define CLINT_MTIME 0x200bff8
#define CLINT_MTIMECMP 0x2004000
#define INTERVAL 10000000

.section .text
.global timervec
.align 2
timervec:
    addi sp, sp, -24
    sd a0, 16(sp)
    sd a1, 8(sp)
    sd t0, 0(sp)

    lui a0, %hi(CLINT_MTIME)     
    addi a0, a0, %lo(CLINT_MTIME) 
    lui a1, %hi(CLINT_MTIMECMP)
    addi a1, a1, %lo(CLINT_MTIMECMP)
    li t0, INTERVAL

    ld a0, 0(a0)  
    add a0, t0, a0
    sd a0, 0(a1)

    ld a0, 16(sp)
    ld a1, 8(sp)
    ld t0, 0(sp)
    addi sp, sp, 24

    mret



