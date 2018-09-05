#ifndef __KERNEL_PCI_CFG_H
#define __KERNEL_PCI_CFG_H

#include "wapi/wapi.h"
#include <stdint.h>

namespace kernel
{
    struct config_accessor
    {
        wapi::node wapi_node = wapi::node(method_delegate(handle_wapi_request),
                                          METHOD_GET_MASK,"cfg");

        virtual uint8_t config_read_8(uint8_t bus, uint8_t devfn,
                                      uint16_t offset) = 0;
        virtual uint16_t config_read_16(uint8_t bus, uint8_t devfn,
                                        uint16_t offset) = 0;
        virtual uint32_t config_read_32(uint8_t bus, uint8_t devfn,
                                        uint16_t offset) = 0;
        virtual uint64_t config_read_64(uint8_t bus, uint8_t devfn,
                                        uint16_t offset) = 0;

        virtual void config_write_8(uint8_t val, uint8_t bus, uint8_t devfn,
                                    uint16_t offset) = 0;
        virtual void config_write_16(uint16_t val, uint8_t bus, uint8_t devfn,
                                     uint16_t offset) = 0;
        virtual void config_write_32(uint32_t val, uint8_t bus, uint8_t devfn,
                                     uint16_t offset) = 0;
        virtual void config_write_64(uint64_t val, uint8_t bus, uint8_t devfn,
                                     uint16_t offset) = 0;

        virtual void handle_wapi_request(wapi::node* node, http::request* req,
                                         json::object* obj,
                                         http::response* rsp) = 0;
    };
}

#endif /* __KERNEL_PCI_CFG_H */
