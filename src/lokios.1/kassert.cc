#include "kassert.h"
#include "kernel_args.h"
#include <stdlib.h>

void
kernel::vgawrite(uint8_t x, uint8_t y, const char* s, uint16_t cflags) noexcept
{
    uint16_t* addr = &((uint16_t*)kernel::kargs->vga_base)[y*80 + x];
    while (*s)
        *addr++ = cflags | *s++;
}

void
kernel::halt() noexcept
{
    vgawrite(74,24,"Halted");
    for (;;)
        asm ("hlt;");
}

extern "C" void
abort()
{
    // Hilite the entire screen red.
    uint64_t* addr = (uint64_t*)kernel::kargs->vga_base;
    for (unsigned int i=0; i<500; ++i)
    {
        uint64_t val = *addr;
        val         &= 0x00FF00FF00FF00FF;
        val         |= 0x4F004F004F004F00;
        *addr++      = val;
    }
    kernel::halt();
}

void
kernel::panic(const char* s) noexcept
{
    // Draw the message in hilited red text at the top-left corner of the
    // screen.
    kernel::vgawrite(0,0,"Kernel panic");
    kernel::vgawrite(0,1,s);
    kernel::halt();
}
