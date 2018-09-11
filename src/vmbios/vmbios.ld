EXTERN(_entry)
SECTIONS
{
    .text :
    {
        *vmb_main.a:*(.text)
    }
    .data :
    {
        *(.data)
    }

    . = 0xFFF0;
    .vmb_entry_text :
    {
        *vmb_entry.a:*(.text)
        FILL(0x00);
        . = 0xFFFF;
        BYTE(0x00);
    }
}
