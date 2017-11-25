#ifndef __KERNEL_CONSOLE_H
#define __KERNEL_CONSOLE_H

#include <stdint.h>

struct console
{
    uint16_t*   base;
    uint16_t    x;
    uint16_t    y;

    void    scroll();
    void    putc(char c);
    void    puts(const char* s);

    console(uint16_t* base);
};

extern console vga;

#endif /* __KERNEL_CONSOLE_H */
