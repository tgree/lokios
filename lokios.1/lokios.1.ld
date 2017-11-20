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
    } = 0xCC

    /* Provide at least 4096 bytes to hold the E820 map.  This should be more
     * than enough (I hope). */
    .e820_map :
    {
        lokios.1/e820.o(.e820_map)
        . += 4096;
        . = ALIGN(4096);
    } = 0xDD
    _e820_last_max_entry = . - 24;

    /* Provide a page table that identity-maps the first 2M of memory via a
     * huge page. */
    .page_table :
    {
        _page_table = .;
        ASSERT(_page_table == ALIGN(4096), "page table misaligned");
        QUAD((_page_table + 4096) | 3);
        . = ALIGN(4096);
        QUAD((_page_table + 8192) | 3);
        . = ALIGN(4096);
        QUAD(0x0000000000000083);
        . = ALIGN(4096);
    }

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
