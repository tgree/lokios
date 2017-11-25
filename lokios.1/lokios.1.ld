SECTIONS
{
    /* This value should match the _kernel_base value in lokios.0.ld. */
    . = 0x00200000;
    .image_header :
    {
        LONG(_num_sectors)
        . = ALIGN(0x10);
    }

    .text :
    {
        lokios.1/entry.o(.text)
        *(.text)
    }

    .eh_frame :
    {
        *(.eh_frame)
    }
    
    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }

    .filler :
    {
        BYTE(0xBB)
        FILL(0xBB)
        . = ALIGN(512);
        BYTE(0xAA)
        FILL(0xAA)
        . = ALIGN(512);
    }

    ASSERT(. % 512 == 0, "not an integral number of sectors")
    _num_sectors = (. - 0x00200000) / 512;
}
