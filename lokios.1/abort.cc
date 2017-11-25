#include "kernel_args.h"
#include <stdlib.h>
#include <stdint.h>

void
abort()
{
    // Hilite the entire screen red.
    uint64_t* addr = (uint64_t*)kargs->vga_base;
    for (unsigned int i=0; i<500; ++i)
    {
        uint64_t val = *addr;
        val         &= 0x00FF00FF00FF00FF;
        val         |= 0x4F004F004F004F00;
        *addr++      = val;
    }

    for (;;)
        asm ("hlt;");
}

void
aborts(const char* s)
{
    // Draw the message in hilited red text at the top-left corner of the
    // screen.
    uint16_t* addr = (uint16_t*)kargs->vga_base;
    while (*s)
        *addr++ = 0x4F00 | *s++;

    for (;;)
        asm ("hlt;");
}
