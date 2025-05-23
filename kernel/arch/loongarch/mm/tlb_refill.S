#define CSR_TLBRSAVE 0x8B
#define CSR_PGD 0x1B

.global tlb_refill
.align 0x4
tlb_refill:
    csrwr   $t0, CSR_TLBRSAVE
    csrrd   $t0, CSR_PGD
    // lddir 新读取的页表基址会带上 pte 的 flag
    // 非常奇怪
    lddir   $t0, $t0, 2
    lddir   $t0, $t0, 1 
    ldpte   $t0, 0
    ldpte   $t0, 1
    tlbfill
    csrrd   $t0, CSR_TLBRSAVE
    ertn