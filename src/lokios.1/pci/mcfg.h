#ifndef __KERNEL_PCI_MCFG_H
#define __KERNEL_PCI_MCFG_H

#include "cfg.h"

namespace kernel
{
    struct mem_config_accessor : public config_accessor
    {
        const uint64_t  base;
        void* const     vbase;
        const uint8_t   start_bus_num;
        const uint8_t   end_bus_num;

        virtual uint8_t config_read_8(uint8_t bus, uint8_t devfn,
                                      uint16_t offset);
        virtual uint16_t config_read_16(uint8_t bus, uint8_t devfn,
                                        uint16_t offset);
        virtual uint32_t config_read_32(uint8_t bus, uint8_t devfn,
                                        uint16_t offset);
        virtual uint64_t config_read_64(uint8_t bus, uint8_t devfn,
                                        uint16_t offset);

        virtual void config_write_8(uint8_t val, uint8_t bus, uint8_t devfn,
                                    uint16_t offset);
        virtual void config_write_16(uint16_t val, uint8_t bus, uint8_t devfn,
                                     uint16_t offset);
        virtual void config_write_32(uint32_t val, uint8_t bus, uint8_t devfn,
                                     uint16_t offset);
        virtual void config_write_64(uint64_t val, uint8_t bus, uint8_t devfn,
                                     uint16_t offset);

        mem_config_accessor(uint64_t base, uint8_t start_bus_num,
                            uint8_t end_bus_num);
    };
}

#endif /* __KERNEL_PCI_MCFG_H */
