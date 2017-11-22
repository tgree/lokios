SECTIONS
{
    /* PXE loads us into memory at 0x7c00, so link ourselves at that base
     * address. */
    . = 0x7c00;
    .bootsector_text :
    {
        lokios.0/bootsector/entry.o(.text)
        lokios.0/bootsector/*(.text)
    }
    .bootsector_data :
    {
        lokios.0/bootsector/*(.data)
        _last_sector = .;
        SHORT(__last_sector);
    }

    /* We'll switch to a 3K stack from 0xF000 - 0xFC00. */
    _stack_bottom = 0xF000;
    _stack_top    = 0xFC00;

    /* Include a valid MBR signature for booting off disk. */
    . = 0x7dfe;
    .mbr_sig :
    {
        BYTE(0x55)
        BYTE(0xAA)
    }

    /* That's the end of the boot sector.  If we PXE-booted the rest of the
     * code is already in memory.  If we MBR-booted then the bootsector has
     * gone and loaded the rest of the code into memory.  In either case, they
     * will then branch into code at or above 0x7E00. */
    . = 0x7e00;
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

    /* Make sure we didn't crash into the stack. */
    ASSERT(. <= 0xF000, "image crashed into the low stack address")

    /* Build the header at the start of the first sector. */
    __last_sector = (. - 0x7c00) / 512;
}
