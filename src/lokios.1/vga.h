#ifndef __KERNEL_VGA_H
#define __KERNEL_VGA_H

#include "console.h"
#include <stdint.h>

namespace kernel
{
    struct vga_console : public kconsole
    {
        uint16_t*   base;
        uint16_t    x;
        uint16_t    y;

        void    scroll();

        // Move to the start of the next line, scrolling if necessary.
        void    putnewline();

        // Put a character or a string.
        virtual void    _putc(char c);

        vga_console(uint16_t* base);
    };

    void init_vga_console();
}

#endif /* __KERNEL_VGA_H */
