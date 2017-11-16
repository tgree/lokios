SECTIONS
{
    /* PXE loads us into memory at 0x7c00, so link ourselves at that base address. */
    . = 0x7c00;
    .text : { *(.text) }

    /* Move other stuff to 0x8000. */
    . = 0x8000;
    .data : { *(.data) }
    .bss  : { *(.bss) }
}
