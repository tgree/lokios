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
    }
}
