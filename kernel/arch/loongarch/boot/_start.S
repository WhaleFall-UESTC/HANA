.section .text._start
.global _start
_start:
    # initialize stack
    la      $sp, init_stack 
    csrrd   $tp, 0x20
    li.d    $t0, 4096
    addi.d  $a0, $tp, 1
    mul.d   $a0, $a0, $t0
    add.d   $sp, $sp, $a0

    bl start

spin:
    b spin

    
    
