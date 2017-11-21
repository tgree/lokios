SECTIONS
{
    /* lokios.0 will load us into memory at 0x7e00, so link ourselves at that
     * base address.  Our entry point is 0x7e10 and there will be a header
     * prepended later in this file.*/
    . = 0x7e10;
    .text :
    {
        *(.text)
    }
    .data :
    {
        *(.data)
    } = 0xCC

    /* Provide a global descriptor table to use for the swtich to 16-bit
     * unreal mode. */
    . = ALIGN(64);
    .gdt_unreal_16 :
    {
        _gdt_unreal_16 = .;
        QUAD(0x0000000000000000);
        QUAD(0x00CF92000000FFFF);

        _gdt_unreal_16_desc = .;
        SHORT(_gdt_unreal_16_desc - _gdt_unreal_16 - 1);
        LONG(_gdt_unreal_16);
    }

    /* Provide a global descriptor table to use for the switch to 64-bit
     * mode. */
    . = ALIGN(64);
    .gdt_64 :
    {
        _gdt_64 = .;
        QUAD(0x0000000000000000);
        QUAD(0x00209A0000000000);   /* Code descriptor */
        QUAD(0x0000920000000000);   /* Data descriptor */

        _gdt_64_desc = .;
        SHORT(_gdt_64_desc - _gdt_64 - 1);
        LONG(_gdt_64);
    }

    /* Provide an empty interrupt descriptor table.  TODO: Figure out what to
     * really put here. */
    . = ALIGN(64);
    .idt_64 :
    {
        _idt_64 = .;
        BYTE(0);
        _idt_64_desc = .;
        SHORT(_idt_64_desc - _idt_64 - 1);
        LONG(_idt_64);
    }

    /* Provide at least 4096 bytes to hold the E820 map.  This should be more
     * than enough (I hope). */
    .e820_map :
    {
        _e820_end = .;
        SHORT(0xFFFF);
        _e820_map = .;
        . += 4096;
        . = ALIGN(4096);
    } = 0xDD
    _e820_last_max_entry = . - 24;

    /* Provide a page table that identity-maps the first 4M of memory via a
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
        QUAD(0x0000000000200083);
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
