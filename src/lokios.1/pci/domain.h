#ifndef __KERNEL_PCI_DOMAIN_H
#define __KERNEL_PCI_DOMAIN_H

#include "cfg.h"
#include "wapi/wapi.h"
#include "k++/klist.h"

namespace kernel::pci
{
    struct dev;

    struct domain
    {
        uint16_t                        id;
        uint8_t                         first_bus_num;
        uint8_t                         last_bus_num;
        struct config_accessor*         cfg;
        kernel::klist<kernel::pci::dev> devices;
        wapi::node                      wapi_node;

        domain(uint16_t id, uint8_t first_bus_num, uint8_t last_bus_num,
               config_accessor* cfg);
    };

    static inline bool operator<(const domain& lhs, const domain& rhs)
    {
        return lhs.id < rhs.id;
    }
}

#endif /* __KERNEL_PCI_DOMAIN_H */
