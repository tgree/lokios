#include "vga.h"
#include "hdr/compiler.h"

extern "C" void _vga_putc(char c) __REG_PARMS__;

void
vga_putc(char c)
{
    if (c == '\n')
        _vga_putc('\r');
    _vga_putc(c);
}
