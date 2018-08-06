#include "vga.h"

void
m32_vga_putc(char c)
{
    if (c == '\n')
        _m32_vga_putc('\r');
    _m32_vga_putc(c);
}
