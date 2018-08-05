#include "serial.h"
#include "hdr/x86.h"
#include <stddef.h>

#define IO_ADDR 0x3F8

static uint8_t
read_reg(uint8_t index)
{
    return inb(IO_ADDR + index);
}

static void
write_reg(uint8_t val, uint8_t index)
{
    outb(val,IO_ADDR + index);
}

void
m32_serial_init()
{
    // Configure for 115200 N/8/1.
    write_reg(read_reg(3) & 0x7F,3);
    write_reg(0x00,1);
    write_reg(0x83,3);
    write_reg(1,0);
    write_reg(0,1);
    write_reg(0x03,3);
    write_reg(0xC7,2);
    write_reg(0x08,4);
}

static void
_m32_serial_putc(char c)
{
    while ((read_reg(5) & 0x20) == 0)
        ;
    outb(c,IO_ADDR);
}

void
m32_serial_putc(char c)
{
    if (c == '\n')
        _m32_serial_putc('\r');
    _m32_serial_putc(c);
}

void
m32_serial_puts(const char* s)
{
    while (*s)
        m32_serial_putc(*s++);
}

void
m32_serial_putx(uint32_t v)
{
    m32_serial_puts("0x");
    for (size_t i=0; i<8; ++i)
    {
        m32_serial_putc("0123456789ABCDEF"[((v >> 28) & 0xF)]);
        v <<= 4;
    }
}

static void
_m32_serial_putu(uint32_t v)
{
    if (v == 0)
        return;
    
    _m32_serial_putu(v/10);
    m32_serial_putc("0123456789"[v % 10]);
}

void
m32_serial_putu(uint32_t v)
{
    if (v == 0)
        m32_serial_putc('0');
    else
        _m32_serial_putu(v);
}
