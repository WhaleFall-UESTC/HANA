.section .text
.globl _start

_start:
    pcaddi  $a0, 0
    addi.d  $a0, $a0, 24
    ori     $a7, $zero, 221
    ori     $a1, $zero, 0
    ori     $a2, $zero, 0

    syscall 0

1:
    b       1b
    nop

.asciz  "/init"

# This is compiled to:
#  04 00 00 18 84 60 c0 02 0b 74 83 03 05 00 80 03
#  06 00 80 03 00 00 2b 00 00 00 00 50 00 00 40 03
#   following the string
