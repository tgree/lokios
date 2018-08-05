#include "vga.h"
#include "hdr/x86.h"
#include <stdint.h>
#include <stddef.h>

void
m32_vga_putc(char c)
{
    if (c == '\n')
        _m32_vga_putc('\r');
    _m32_vga_putc(c);
}

void
m32_vga_puts(const char* s)
{
    while (*s)
        m32_vga_putc(*s++);
}

void
m32_vga_putx(uint32_t v)
{
    m32_vga_puts("0x");
    for (size_t i=0; i<8; ++i)
    {
        m32_vga_putc("0123456789ABCDEF"[((v >> 28) & 0xF)]);
        v <<= 4;
    }
}

void
m32_vga_putx8(uint8_t v)
{
    for (size_t i=0; i<2; ++i)
    {
        m32_vga_putc("0123456789ABCDEF"[((v >> 4) & 0xF)]);
        v <<= 4;
    }
}

static void
_m32_vga_putu(uint32_t v)
{
    if (v == 0)
        return;
    
    _m32_vga_putu(v/10);
    m32_vga_putc("0123456789"[v % 10]);
}

void
m32_vga_putu(uint32_t v)
{
    if (v == 0)
        m32_vga_putc('0');
    else
        _m32_vga_putu(v);
}

void
m32_vga_hexdump(const void* _p, uint16_t n)
{
    const uint8_t* p = (const uint8_t*)_p;
    uint32_t i;
    for (i=0; i<n; ++i)
    {
        m32_vga_putx8(*p++);
        if ((i+1) % 16 == 0)
            m32_vga_putc('\n');
        else
            m32_vga_putc(' ');
    }
    if (i % 16)
        m32_vga_putc('\n');
}
