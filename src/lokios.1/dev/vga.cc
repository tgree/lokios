#include "vga.h"
#include "mm/mm.h"
#include "k++/deferred_init.h"

#define CONSOLE_HEIGHT  25
#define CONSOLE_WIDTH   79

#define SCREEN_HEIGHT   25
#define SCREEN_WIDTH    80

static kernel::deferred_global<kernel::vga_console> vga;
static uint16_t* vga_base;
static uint16_t vga_colors = 0x1F00;

void
kernel::init_vga_console(dma_addr64 _vga_base)
{
    vga_base = (uint16_t*)phys_to_virt(_vga_base);
    vga.init(vga_base);
    for (size_t x=CONSOLE_WIDTH; x<SCREEN_WIDTH; ++x)
        for (size_t y=0; y<SCREEN_HEIGHT; ++y)
            vga_base[y*SCREEN_WIDTH + x] = 0x0F00 | ' ';
    kernel::console::register_console(vga);
}

size_t
kernel::vga_write(uint8_t x, uint8_t y, const char* s, uint16_t cflags) noexcept
{
    uint16_t* base = &vga_base[y*SCREEN_WIDTH + x];
    uint16_t* addr = base;
    while (*s)
        *addr++ = cflags | *s++;
    return addr - base;
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

void
kernel::vga_set_colors(uint16_t cflags)
{
    vga_colors = cflags;
    uint16_t* addr = &vga_base[(SCREEN_HEIGHT-1)*SCREEN_WIDTH];
    for (size_t x=0; x<CONSOLE_WIDTH; ++x)
    {
        uint16_t val = *addr;
        val         &= 0x00FF;
        val         |= cflags;
        *addr++      = val;
    }
}

kernel::vga_console::vga_console(uint16_t* base):
    base(base),
    x(0),
    y(0)
{
    for (size_t x=0; x<CONSOLE_WIDTH; ++x)
        for (size_t y=0; y<CONSOLE_HEIGHT; ++y)
            base[y*SCREEN_WIDTH + x] = 0x1F00 | ' ';
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
        *dst++ = vga_colors | ' ';
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
        base[y*SCREEN_WIDTH + x] = vga_colors | (uint16_t)c;
        if (++x == CONSOLE_WIDTH)
            putnewline();
    }
}
