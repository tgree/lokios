#include "vga.h"
#include "kernel_args.h"
#include <new>

static kernel::vga_console* vga;
static uint64_t _vga[(sizeof(*vga) + sizeof(uint64_t) - 1)/sizeof(uint64_t)];

void
kernel::init_vga_console()
{
    vga = new(_vga) vga_console((uint16_t*)kernel::kargs->vga_base);
    kernel::console::register_console(vga);
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
    uint64_t* src = (uint64_t*)&base[80];
    uint64_t* dst = (uint64_t*)base;
    for (unsigned int i=0; i<480; ++i)
        *dst++ = *src++;
    for (unsigned int i=480; i<500; ++i)
        *dst++ = 0x1F201F201F201F20;
}

void
kernel::vga_console::putnewline()
{
    x = 0;
    if (++y == 25)
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
        base[y*80 + x] = 0x1F00 | (uint16_t)c;
        if (++x == 80)
            putnewline();
    }
}
