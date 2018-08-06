#include "vga.h"
#include "hdr/compiler.h"

extern "C" void _m32_vga_putc(char c) __REG_PARMS__;

void
m32_vga_putc(char c)
{
    if (c == '\n')
        _m32_vga_putc('\r');
    _m32_vga_putc(c);
}
