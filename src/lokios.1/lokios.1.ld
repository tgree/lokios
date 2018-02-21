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
        *lokios.1/entry.o(.text)
        *crtbeginS.o(.text)
        *lokios.1/init.o(.text)
        *(.text)
        *crtendS.o(.text)
    }

    .init :
    {
        *lokios.1/crti.o(.init)
        KEEP (*(SORT_NONE(.init)))
        *lokios.1/crtn.o(.init)
    }

    .fini :
    {
        *lokios.1/crti.o(.fini)
        KEEP (*(SORT_NONE(.fini)))
        *lokios.1/crtn.o(.fini)
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
        _eh_frame_begin = .;
        *(.eh_frame)
        QUAD(0);
    }
    
    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }

    .tdata :
    {
        _tdata_begin = .;
        *(.tdata .tdata.* .gnu.linkonce.td.*)
        _tdata_end = .;
    }
    _tdata_size = SIZEOF(.tdata);

    .tbss :
    {
        _tbss_begin = .;
        *(.tbss .tbss.* .gnu.linkonce.tb.*)
        *(.tcommon)
        _tbss_end = .;
    }
    _tbss_size = ALIGN(SIZEOF(.tbss), ALIGNOF(.tdata));
    ASSERT(_tdata_size + _tbss_size < 0x0800, "TLS size overflow!")

    .filler :
    {
        BYTE(0xBB)
        FILL(0xBB)
        . = ALIGN(4096);
    }

    ASSERT(. % 4096 == 0, "not an integral number of sectors")
    _num_sectors = (. - 0x00200000) / 512;
    _sbrk = .;
}
