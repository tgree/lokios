#include "massert.h"
#include "serial.h"
#include "hdr/x86.h"

void
m32_abort(const char* f, int l)
{
    m32_serial_puts("ASSERTION FAILED: ");
    m32_serial_puts(f);
    m32_serial_putc(':');
    m32_serial_putu(l);
    m32_serial_putc('\n');
    cpu_halt();
    for (;;)
        ;
}
