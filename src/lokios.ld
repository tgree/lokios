SECTIONS
{
    /* PXE loads us into memory at 0x7c00, so link ourselves at that base
     * address. */
    . = 0x7c00;
    .text :
    {
        entry.o(.text)
        *(.text)
    }

    /* Move other stuff to 0x8000. */
    /*. = 0x8000; */
    .data : { *(.data) }
    .bss  : { *(.bss) }

    /* Include a valid MBR signature so we can test booting off disk. */
    . = 0x7dfe;
    .mbr_sig :
    {
        BYTE(0x55)
        BYTE(0xAA)
    }
}
