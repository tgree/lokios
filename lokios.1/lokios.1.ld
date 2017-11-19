SECTIONS
{
    /* lokios.0 will load us into memory at 0x7e00, so link ourselves at that
     * base address.  Our entry point is 0x7e10 and there will be a header
     * prepended later in this file.*/
    . = 0x7e10;
    .text :
    {
        lokios.1/entry.o(.text)
        *(.text)
    }
    .data :
    {
        *(.data)
        . = ALIGN(512);
    } = 0xCC
    _heap_start = .;
    _heap_end = 0xFC00;

    /* Build the header at the start of the first sector. */
    _last_sector = ((. - 0x7e00) / 512) + 1;
    . = 0x7e00;
    .lokios_1_hdr :
    {
        /* 'Tery' */
        BYTE(0x54)
        BYTE(0x65)
        BYTE(0x72)
        BYTE(0x79)

        /* Last sector number of file. */
        SHORT(_last_sector)

        /* Unused */
        SHORT(0)
        LONG(0)
        LONG(0)
    }
}
