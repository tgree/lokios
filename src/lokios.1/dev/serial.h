#ifndef __KERNEL_DEV_SERIAL_H
#define __KERNEL_DEV_SERIAL_H

#include "kernel/console.h"

namespace kernel
{
    enum serial_config
    {
        N81_115200 = 0,
    };

    class serial_console : public kconsole
    {
        const uint16_t  ioaddr;

                uint8_t read_reg(uint8_t index);
                void    write_reg(uint8_t val, uint8_t index);
                void    __putc(char c);
                void    putnewline();
        virtual void    _putc(char c);

    public:
        serial_console(uint16_t ioaddr, serial_config config);
    };

    void serial_write(const char* s) noexcept;

    void init_serial_console(uint16_t ioaddr, serial_config config);
}

#endif /* __KERNEL_DEV_SERIAL_H */
