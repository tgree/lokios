#ifndef __MODE32_VGA_H
#define __MODE32_VGA_H

#include "hdr/compiler.h"
#include <stdint.h>

extern "C"
{
    void _m32_vga_putc(char c) __REG_PARMS__;
    void m32_vga_putc(char c);
    void m32_vga_puts(const char* s);
    void m32_vga_putx(uint32_t v);
    void m32_vga_putx8(uint8_t v);
    void m32_vga_putu(uint32_t v);
    void m32_vga_hexdump(const void* p, uint16_t n);
    void m32_vga_init();
}

#endif /* __MODE32_VGA_H */
