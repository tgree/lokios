#include "console.h"
#include "serial.h"
#include "vga.h"
#include "k++/kprintf.h"

static
void _putc(void* cookie, char c)
{
    m32_serial_putc(c);
    m32_vga_putc(c);
}

void
console::init()
{
    m32_serial_init();
}

void
console::vprintf(const char* fmt, va_list ap)
{
    kernel::kvprintf(_putc,0,fmt,ap);
}
