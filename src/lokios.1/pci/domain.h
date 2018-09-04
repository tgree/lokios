#ifndef __KERNEL_PCI_DOMAIN_H
#define __KERNEL_PCI_DOMAIN_H

#include "cfg.h"
#include "k++/klist.h"

namespace kernel::pci
{
    struct dev;

    struct domain
    {
        uint16_t                        id;
        struct config_accessor*         cfg;
        kernel::klist<kernel::pci::dev> devices;
    };

    static inline bool operator<(const domain& lhs, const domain& rhs)
    {
        return lhs.id < rhs.id;
    }

    void wapi_register(domain* d);
}

#endif /* __KERNEL_PCI_DOMAIN_H */
