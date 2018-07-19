#include "vga.h"
#include "mm/mm.h"
#include <new>

#define CONSOLE_HEIGHT  25
#define CONSOLE_WIDTH   79

#define SCREEN_HEIGHT   25
#define SCREEN_WIDTH    80

static kernel::vga_console* vga;
static uint64_t _vga[(sizeof(*vga) + sizeof(uint64_t) - 1)/sizeof(uint64_t)];
static uint16_t* vga_base;

void
kernel::init_vga_console(dma_addr64 _vga_base)
{
    vga_base = (uint16_t*)phys_to_virt(_vga_base);
    vga = new(_vga) vga_console(vga_base);
    kernel::console::register_console(vga);
}

void
kernel::vga_write(uint8_t x, uint8_t y, const char* s, uint16_t cflags) noexcept
{
    uint16_t* addr = &vga_base[y*SCREEN_WIDTH + x];
    while (*s)
        *addr++ = cflags | *s++;
}

void
kernel::vga_set_flags(uint16_t _cflags) noexcept
{
    uint64_t* addr  = (uint64_t*)vga_base;
    uint64_t cflags = ((uint64_t)_cflags << 48) |
                      ((uint64_t)_cflags << 32) |
                      ((uint64_t)_cflags << 16) |
                      ((uint64_t)_cflags <<  0);
    for (unsigned int i=0; i<500; ++i)
    {
        uint64_t val = *addr;
        val         &= 0x00FF00FF00FF00FF;
        val         |= cflags;
        *addr++      = val;
    }
}

kernel::vga_console::vga_console(uint16_t* base):
    base(base),
    x(0),
    y(0)
{
    uint64_t* addr = (uint64_t*)base;
    for (unsigned int i=0; i<500; ++i)
        *addr++ = 0x1F201F201F201F20;
}

void
kernel::vga_console::scroll()
{
    for (size_t y=0; y<CONSOLE_HEIGHT-1; ++y)
    {
        uint16_t* src = (uint16_t*)&base[(y+1)*SCREEN_WIDTH];
        uint16_t* dst = (uint16_t*)&base[y*SCREEN_WIDTH];
        for (size_t x=0; x<CONSOLE_WIDTH; ++x)
            *dst++ = *src++;
    }

    uint16_t* dst = &base[(CONSOLE_HEIGHT-1)*SCREEN_WIDTH];
    for (size_t x=0; x<CONSOLE_WIDTH; ++x)
        *dst++ = 0x1F20;
}

void
kernel::vga_console::putnewline()
{
    x = 0;
    if (++y == CONSOLE_HEIGHT)
    {
        --y;
        scroll();
    }
}

void
kernel::vga_console::_putc(char c)
{
    if (c == '\n')
        putnewline();
    else
    {
        base[y*SCREEN_WIDTH + x] = 0x1F00 | (uint16_t)c;
        if (++x == CONSOLE_WIDTH)
            putnewline();
    }
}
