EXTERN(_start)
SECTIONS
{
    /* PXE loads us into memory at 0x7c00, so link ourselves at that base
     * address. */
    . = 0x7C00;
    .bootsector_text :
    {
        *bootsector.a:entry.o(.text)
        *bootsector.a:*(*text)
    }
    .bootsector_data ALIGN(1) :
    {
        *timestamp.o(.data)
        *bootsector.a:*(.data)
    }
    ASSERT(. <= 0x7DB4, "boot sector crashed into disk ID")

    /* Include a valid MBR signature for booting off disk. */
    . = 0x7DFE;
    .mbr_sig :
    {
        BYTE(0x55)
        BYTE(0xAA)
    }

    /* Space for 0x7E00-0x7FFF is currently unused. */

    /* That's the end of the boot sector.  If we PXE-booted the rest of the
     * code is already in memory.  If we MBR-booted then the bootsector has
     * gone and loaded the rest of the code into memory.  In either case, they
     * will then branch into code at or above 0x8000.  We align the _smp_entry
     * routine at 0x8000 because the MP SIPI vector requires page alignment and
     * we want this to all be mapped read-only by the time lokios.1 is up and
     * running.  Due to alignment of other sections such as page table pages,
     * bumping the .text base address from 0x7E00 to 0x8000 doesn't actually
     * increase the size of the image. */
    . = 0x8000;
    .text :
    {
        *smp_entry.o(.text)
        *(.text)
    }
    .data :
    {
        *(.data)
    } = 0xCC

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

    .bss :
    {
        *(.bss)
    }

    /* Provide at least 4096 bytes to hold the E820 map.  This should be more
     * than enough (I hope).  Before we've loaded the E820 map, this region of
     * memory can also be used for a disk IO bounce buffer. */
    .e820_map :
    {
        _pre_e820_bounce_buffer = .;
        _e820_end = .;
        SHORT(0xFFFF);
        _e820_map = .;
        . += 4096;
        . = ALIGN(4096);
    } = 0xDD
    _e820_last_max_entry = . - 24;

    /* We'll switch to a 3K stack from 0xF000 - 0xFC00. */
    _stack_bottom = 0xF000;
    _stack_top    = 0xFC00;

    /* Make sure we didn't crash into the stack. */
    ASSERT(. <= _stack_bottom, "image crashed into the low stack address")

    /* Compute the number of sectors the MBR loader needs to read. */
    _extra_sectors = (. - 0x7e00) / 512;

    /* This is where we are going to physically load the kernel. */
    _elf_base = 0x00200000;
    _kernel_base = _elf_base + 0x200;
    _kernel_bsp_entry = _kernel_base + 0x100;
    _kernel_ap_entry = _kernel_base + 0x200;
    _kernel_stack = 0x001FF000;
    _kernel_params = _kernel_stack;
}
