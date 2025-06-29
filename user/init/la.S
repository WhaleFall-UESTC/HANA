.section .init
.global _start
_start:
    bl main
loop:
    b loop
