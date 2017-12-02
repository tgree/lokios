#ifndef __KERNEL_CONSOLE_H
#define __KERNEL_CONSOLE_H

#include "char_stream.h"
#include <stdint.h>

namespace kernel
{
    struct console : public char_stream
    {
        uint16_t*   base;
        uint16_t    x;
        uint16_t    y;

        void    scroll();

        // Move to the start of the next line, scrolling if necessary.
        void    putnewline();

        // Put a character or a string.
        virtual void    _putc(char c);

        console(uint16_t* base);
    };

    extern console* vga;

    void init_console();
}

#endif /* __KERNEL_CONSOLE_H */
