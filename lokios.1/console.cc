#include "console.h"
#include "kernel_args.h"

static uint16_t* console_base;
static uint16_t console_x;
static uint16_t console_y;

void
console_init()
{
    console_base = (uint16_t*)kargs->vga_base;
    console_x    = 0;
    console_y    = 0;

    uint64_t* addr = (uint64_t*)console_base;
    for (unsigned int i=0; i<500; ++i)
        addr[i] = 0x1F201F201F201F20;
}

void
console_scroll()
{
    uint64_t* src = (uint64_t*)&console_base[80];
    uint64_t* dst = (uint64_t*)console_base;
    for (unsigned int i=0; i<420; ++i)
        *dst++ = *src;
    for (unsigned int i=420; i<500; ++i)
        *dst++ = 0x1F201F201F201F20;
}

void
console_putc(char c)
{
    console_base[console_y*80 + console_x] = 0x1F00 | (uint16_t)c;
    if (++console_x == 80)
    {
        console_x = 0;
        if (++console_y == 25)
        {
            --console_y;
            console_scroll();
        }
    }
}

void
console_puts(const char* s)
{
    while (*s)
        console_putc(*s++);
}
