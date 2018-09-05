#ifndef __KERNEL_PCI_IOCFG_H
#define __KERNEL_PCI_IOCFG_H

#include "cfg.h"

namespace kernel
{
    struct io_config_accessor : public config_accessor
    {
        const uint16_t  config_addr;
        const uint16_t  config_data;

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

        virtual void handle_wapi_request(wapi::node* node, http::request* req,
                                         json::object* obj,
                                         http::response* rsp);

        io_config_accessor(uint16_t config_addr, uint16_t config_data);
    };
}

#endif /* __KERNEL_PCI_IOCFG_H */
