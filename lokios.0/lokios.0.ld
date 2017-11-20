SECTIONS
{
    /* PXE loads us into memory at 0x7c00, so link ourselves at that base
     * address. */
    . = 0x7c00;
    .text :
    {
        lokios.0/entry.o(.text)
        *(.text)
    }
    .data : { *(.data) }
    .bss  : { *(.bss) }

    /* We'll switch to a 3K stack from 0xF000 - 0xFC00. */
    _stack_bottom = 0xF000;
    _stack_top    = 0xFC00;

    /* Include a valid MBR signature so we can test booting off disk. */
    . = 0x7dfe;
    .mbr_sig :
    {
        BYTE(0x55)
        BYTE(0xAA)
    }
}
