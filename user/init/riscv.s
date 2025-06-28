.section .init
.global _start
_start:
    call main
loop:
    j loop
