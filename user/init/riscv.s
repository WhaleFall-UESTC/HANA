.section .text
.globl _start
_start:
    auipc a0, 0x0
    addi a0, a0, 28
    li a7, 221
    li a1, 0
    li a2, 0

    ecall
1:
    j 1b

.string "/init"

# This is compiled to:
#   17 05 00 00 13 05 c5 01 93 08 d0 0d 93 05 00 00 13 06 00 00 73 00 00 00 6f 00 00 00
#   following the string
