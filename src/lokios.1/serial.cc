#include "serial.h"
#include <new>

struct uart_settings
{
    kernel::serial_config   config;
    uint16_t                divisor;
    uint8_t                 line_control;
    uint8_t                 fifo_control;
    uint8_t                 modem_control;
};

static kernel::serial_console* serial;
static uint64_t _serial[(sizeof(*serial) + sizeof(uint64_t) - 1)/
                        sizeof(uint64_t)];

static const uart_settings settings_map[] = {
    {kernel::N81_115200,1,0x03,0xC7,0x0B},
};

void
kernel::init_serial_console(uint16_t ioaddr, serial_config config)
{
    serial = new(_serial) serial_console(ioaddr,config);
    kernel::console::register_console(serial);
}

void
kernel::serial_write(const char* s) noexcept
{
    if (!serial)
        return;

    serial->printf("%s",s);
}

kernel::serial_console::serial_console(uint16_t ioaddr, serial_config config):
    ioaddr(ioaddr)
{
    // Find a matching configuration.
    const uart_settings* cfg = NULL;
    for (auto& c : settings_map)
    {
        if (c.config == config)
            cfg = &c;
    }
    kassert(cfg != NULL);

    // Disable interrupts.
    write_reg(read_reg(3) & 0x7F,3);
    write_reg(0x00,1);

    // Apply the configuration.
    write_reg(0x80 | cfg->line_control,3);
    write_reg((cfg->divisor >> 0),0);
    write_reg((cfg->divisor >> 8),1);
    write_reg(cfg->line_control,3);
    write_reg(cfg->fifo_control,2);
    write_reg(cfg->modem_control,4);
}

uint8_t
kernel::serial_console::read_reg(uint8_t index)
{
    kassert(index < 8);
    return inb(ioaddr + index);
}

void
kernel::serial_console::write_reg(uint8_t val, uint8_t index)
{
    kassert(index < 8);
    outb(val,ioaddr + index);
}

void
kernel::serial_console::__putc(char c)
{
    while ((read_reg(5) & 0x20) == 0)
        ;

    outb(c,ioaddr);
}

void
kernel::serial_console::putnewline()
{
    __putc('\r');
    __putc('\n');
}

void
kernel::serial_console::_putc(char c)
{
    if (c == '\n')
        putnewline();
    else
        __putc(c);
}
