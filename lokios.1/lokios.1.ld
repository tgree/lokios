SECTIONS
{
    . = 0x00200000;
    .image_header :
    {
        LONG(_num_sectors)
        . += 0x10;
    }

    .text :
    {
        *(.text)
    }

    .filler :
    {
        BYTE(0xBB)
        . = ALIGN(512);
    } = 0xAA

    ASSERT(. % 512 == 0, "not an integral number of sectors")
    _num_sectors = (. - 0x00200000) / 512;
}
