#define CSR_SAVE0 0x30
#define CSR_SAVE1 0x31
#define CSR_SAVE2 0x32
#define CSR_SAVE3 0x33
#define CSR_SAVE4 0x34
#define CSR_SAVE5 0x35
#define CSR_SAVE6 0x36
#define CSR_ESTAT 0x5    
#define CSR_ERA   0x6   
#define CSR_BADV  0x7    
#define CSR_BADI  0x8    

#define CSR_PGDL  0x19

#define TRAPFRAME_OFFSET 40

.section trampsec
.globl trampoline
trampoline:
.align 4
.globl uservec
uservec:
    # just for debug
    csrwr   $t0, CSR_SAVE1
    csrwr   $t1, CSR_SAVE2
    csrwr   $t2, CSR_SAVE3
    csrrd   $t0, CSR_ESTAT  // ESTAT
    csrrd   $t1, CSR_ERA    // ERA
    csrrd   $t2, CSR_BADV   // BADV
    csrwr   $t0, CSR_SAVE1
    csrwr   $t1, CSR_SAVE2
    csrwr   $t2, CSR_SAVE3
    
    csrwr   $a0, CSR_SAVE0

    st.d    $ra, $a0, 0+TRAPFRAME_OFFSET
    st.d    $tp, $a0, 8+TRAPFRAME_OFFSET
    st.d    $sp, $a0, 16+TRAPFRAME_OFFSET
    st.d    $a1, $a0, 32+TRAPFRAME_OFFSET
    st.d    $a2, $a0, 40+TRAPFRAME_OFFSET
    st.d    $a3, $a0, 48+TRAPFRAME_OFFSET
    st.d    $a4, $a0, 56+TRAPFRAME_OFFSET
    st.d    $a5, $a0, 64+TRAPFRAME_OFFSET
    st.d    $a6, $a0, 72+TRAPFRAME_OFFSET
    st.d    $a7, $a0, 80+TRAPFRAME_OFFSET
    st.d    $t0, $a0, 88+TRAPFRAME_OFFSET
    st.d    $t1, $a0, 96+TRAPFRAME_OFFSET
    st.d    $t2, $a0, 104+TRAPFRAME_OFFSET
    st.d    $t3, $a0, 112+TRAPFRAME_OFFSET
    st.d    $t4, $a0, 120+TRAPFRAME_OFFSET
    st.d    $t5, $a0, 128+TRAPFRAME_OFFSET
    st.d    $t6, $a0, 136+TRAPFRAME_OFFSET
    st.d    $t7, $a0, 144+TRAPFRAME_OFFSET
    st.d    $t8, $a0, 152+TRAPFRAME_OFFSET
    st.d    $fp, $a0, 160+TRAPFRAME_OFFSET
    st.d    $s0, $a0, 168+TRAPFRAME_OFFSET
    st.d    $s1, $a0, 176+TRAPFRAME_OFFSET
    st.d    $s2, $a0, 184+TRAPFRAME_OFFSET
    st.d    $s3, $a0, 192+TRAPFRAME_OFFSET
    st.d    $s4, $a0, 200+TRAPFRAME_OFFSET
    st.d    $s5, $a0, 208+TRAPFRAME_OFFSET
    st.d    $s6, $a0, 216+TRAPFRAME_OFFSET
    st.d    $s7, $a0, 224+TRAPFRAME_OFFSET
    st.d    $s8, $a0, 232+TRAPFRAME_OFFSET
    
    # user a0 stored in SAVE0
    csrrd   $t0, CSR_SAVE0
    st.d    $t0, $a0, 24+TRAPFRAME_OFFSET

    # restore kernel sp
    ld.d    $sp, $a0, 8

    # tp hold current hartid
    ld.d    $tp, $a0, 32

    # load address of user_trap()
    ld.d    $t0, $a0, 16
    # jump to user_trap
    jirl    $zero ,$t0, 0




.align 4
.globl userret
userret:
    csrrd   $t0, CSR_PGDL
    beq     $t0, $a1, load
    csrwr   $a1, CSR_PGDL
    invtlb  0x0, $zero, $zero


load:
    # load a0 in trapframe, and restore it in SAVE0
    ld.d    $t0, $a0, 24+TRAPFRAME_OFFSET
    csrwr   $t0, CSR_SAVE0

    ld.d    $ra, $a0, 0+TRAPFRAME_OFFSET
    ld.d    $tp, $a0, 8+TRAPFRAME_OFFSET
    ld.d    $sp, $a0, 16+TRAPFRAME_OFFSET
    ld.d    $a1, $a0, 32+TRAPFRAME_OFFSET
    ld.d    $a2, $a0, 40+TRAPFRAME_OFFSET
    ld.d    $a3, $a0, 48+TRAPFRAME_OFFSET
    ld.d    $a4, $a0, 56+TRAPFRAME_OFFSET
    ld.d    $a5, $a0, 64+TRAPFRAME_OFFSET
    ld.d    $a6, $a0, 72+TRAPFRAME_OFFSET
    ld.d    $a7, $a0, 80+TRAPFRAME_OFFSET
    ld.d    $t0, $a0, 88+TRAPFRAME_OFFSET
    ld.d    $t1, $a0, 96+TRAPFRAME_OFFSET
    ld.d    $t2, $a0, 104+TRAPFRAME_OFFSET
    ld.d    $t3, $a0, 112+TRAPFRAME_OFFSET
    ld.d    $t4, $a0, 120+TRAPFRAME_OFFSET
    ld.d    $t5, $a0, 128+TRAPFRAME_OFFSET
    ld.d    $t6, $a0, 136+TRAPFRAME_OFFSET
    ld.d    $t7, $a0, 144+TRAPFRAME_OFFSET
    ld.d    $t8, $a0, 152+TRAPFRAME_OFFSET
    ld.d    $fp, $a0, 160+TRAPFRAME_OFFSET
    ld.d    $s0, $a0, 168+TRAPFRAME_OFFSET
    ld.d    $s1, $a0, 176+TRAPFRAME_OFFSET
    ld.d    $s2, $a0, 184+TRAPFRAME_OFFSET
    ld.d    $s3, $a0, 192+TRAPFRAME_OFFSET
    ld.d    $s4, $a0, 200+TRAPFRAME_OFFSET
    ld.d    $s5, $a0, 208+TRAPFRAME_OFFSET
    ld.d    $s6, $a0, 216+TRAPFRAME_OFFSET
    ld.d    $s7, $a0, 224+TRAPFRAME_OFFSET
    ld.d    $s8, $a0, 232+TRAPFRAME_OFFSET

    # restore user a0, and save TRAPFRAME in SAVE0
    csrwr   $a0, CSR_SAVE0

    ertn