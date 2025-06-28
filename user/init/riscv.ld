ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x0, LENGTH = 64K
}

SECTIONS {
    . = 0x0;
    .text : {
        *(.init)
        *(.text .text.*)
    }
    .data : { *(.data) }
    .bss : { *(.bss) }
}
