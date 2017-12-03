#include "console.h"
#include "kernel_args.h"
#include "sbrk.h"
#include <new>

kernel::console* kernel::vga;
static uint64_t _vga[(sizeof(*kernel::vga) + sizeof(uint64_t) - 1)/
                     sizeof(uint64_t)];

void
kernel::init_console()
{
    kernel::vga = new(_vga) console((uint16_t*)kernel::kargs->vga_base);
}

kernel::console::console(uint16_t* base):
    base(base),
    x(0),
    y(0)
{
    uint64_t* addr = (uint64_t*)base;
    for (unsigned int i=0; i<500; ++i)
        *addr++ = 0x1F201F201F201F20;
}

void
kernel::console::scroll()
{
    uint64_t* src = (uint64_t*)&base[80];
    uint64_t* dst = (uint64_t*)base;
    for (unsigned int i=0; i<480; ++i)
        *dst++ = *src++;
    for (unsigned int i=480; i<500; ++i)
        *dst++ = 0x1F201F201F201F20;
}

void
kernel::console::putnewline()
{
    x = 0;
    if (++y == 25)
    {
        --y;
        scroll();
    }
}

void
kernel::console::_putc(char c)
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
