OUTPUT_ARCH("loongarch64")
ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(5);  /* RX */
    rodata PT_LOAD FLAGS(4); /* R */
    data PT_LOAD FLAGS(6);  /* RW */
    bss PT_LOAD FLAGS(6);   /* RW */
}

SECTIONS {
    . = 0x80000000;

    .text : {
        *(.text._start)
        *(.text .text.*)
        . = ALIGN(0x1000);
        PROVIDE(etext = .);
    } :text


    .rodata : {
        . = ALIGN(16);
        *(.srodata .srodata.*)
        . = ALIGN(16);
        *(.rodata .rodata.*)
    } :rodata

    .data : {
        . = ALIGN(16);
        *(.sdata .sdata.*)
        . = ALIGN(16);
        *(.data .data.*)
    } :data

    .bss : {
        . = ALIGN(16);
        *(.sbss .sbss.*)
        . = ALIGN(16);
        *(.bss .bss.*)
    } :bss

    PROVIDE(end = .);
}
