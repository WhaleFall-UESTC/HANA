.global init_stack
.align  12 
init_stack:
    .space  0x1000
    .align 12

.global main
main:
    add.d $zero, $zero, $zero
spin:
    b spin

