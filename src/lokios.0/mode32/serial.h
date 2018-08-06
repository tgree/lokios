#ifndef __MODE32_SERIAL_H
#define __MODE32_SERIAL_H

#include <stdint.h>

extern "C"
{
    void m32_serial_putc(char c);
    void m32_serial_init();
}

#endif /* __MODE32_SERIAL_H */
