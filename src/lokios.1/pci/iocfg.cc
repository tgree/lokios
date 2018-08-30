#include "iocfg.h"
#include "kern/console.h"

using kernel::console::printf;

static inline uint32_t
config_addr_val(uint8_t bus, uint8_t devfn, uint16_t offset)
{
    return 0x80000000                 |
           (((uint32_t)bus)    << 16) |
           (((uint32_t)devfn)  <<  8) |
           (((uint32_t)offset) <<  0);
}

kernel::io_config_accessor::io_config_accessor(uint16_t config_addr,
    uint16_t config_data):
        config_addr(config_addr),
        config_data(config_data)
{
    printf("IOCFG %04X:%04X\n",config_addr,config_data);
}

uint8_t
kernel::io_config_accessor::config_read_8(uint8_t bus, uint8_t devfn,
    uint16_t offset)
{
    outl(config_addr_val(bus,devfn,offset & ~3),config_addr);
    return inb(config_data + (offset & 3));
}

uint16_t
kernel::io_config_accessor::config_read_16(uint8_t bus, uint8_t devfn,
    uint16_t offset)
{
    kassert((offset & 1) == 0);
    outl(config_addr_val(bus,devfn,offset & ~3),config_addr);
    return inw(config_data + (offset & 3));
}

uint32_t
kernel::io_config_accessor::config_read_32(uint8_t bus, uint8_t devfn,
    uint16_t offset)
{
    kassert((offset & 3) == 0);
    outl(config_addr_val(bus,devfn,offset & ~3),config_addr);
    return inl(config_data + (offset & 3));
}

uint64_t
kernel::io_config_accessor::config_read_64(uint8_t bus, uint8_t devfn,
    uint16_t offset)
{
    kernel::panic("iocfg 64-bit read accesses are forbidden!");
}

void
kernel::io_config_accessor::config_write_8(uint8_t val, uint8_t bus,
    uint8_t devfn, uint16_t offset)
{
    outl(config_addr_val(bus,devfn,offset & ~3),config_addr);
    outb(val,config_data + (offset & 3));
}

void
kernel::io_config_accessor::config_write_16(uint16_t val, uint8_t bus,
    uint8_t devfn, uint16_t offset)
{
    kassert((offset & 1) == 0);
    outl(config_addr_val(bus,devfn,offset & ~3),config_addr);
    outw(val,config_data + (offset & 3));
}

void
kernel::io_config_accessor::config_write_32(uint32_t val, uint8_t bus,
    uint8_t devfn, uint16_t offset)
{
    kassert((offset & 3) == 0);
    outl(config_addr_val(bus,devfn,offset & ~3),config_addr);
    outl(val,config_data + (offset & 3));
}

void
kernel::io_config_accessor::config_write_64(uint64_t val, uint8_t bus,
    uint8_t devfn, uint16_t offset)
{
    kernel::panic("iocfg 64-bit write accesses are forbidden!");
}
