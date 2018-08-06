#ifndef __MODE32_VGA_H
#define __MODE32_VGA_H

#include "hdr/compiler.h"
#include <stdint.h>

extern "C"
{
    void _m32_vga_putc(char c) __REG_PARMS__;
    void m32_vga_putc(char c);
}

#endif /* __MODE32_VGA_H */
