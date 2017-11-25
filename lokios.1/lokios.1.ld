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

    .init :
    {
        KEEP (*(SORT_NONE(.init)))
    }

    .fini :
    {
        KEEP (*(SORT_NONE(.fini)))
    }

    .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    }

    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) \
              SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP (*(.init_array \
                EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) \
                .ctors))
        PROVIDE_HIDDEN (__init_array_end = .);
    }

    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) \
                SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP (*(.fini_array \
                EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) \
                .dtors))
        PROVIDE_HIDDEN (__fini_array_end = .);
    }

    .rodata :
    {
        *(.rodata)
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
