#ifndef __KERNEL_DEV_VGA_H
#define __KERNEL_DEV_VGA_H

#include "kern/console.h"
#include "kern/types.h"

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

    size_t vga_write(uint8_t x, uint8_t y, const char* s,
                     uint16_t cflags = 0x4F00) noexcept;
    void vga_set_flags(uint16_t cflags) noexcept;
    void vga_set_colors(uint16_t cflags);
    void init_vga_console(dma_addr64 vga_base);
}

#endif /* __KERNEL_DEV_VGA_H */
