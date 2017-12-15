#ifndef __KERNEL_PCI_DOMAIN_H
#define __KERNEL_PCI_DOMAIN_H

#include "cfg.h"

namespace kernel::pci
{
    struct domain
    {
        uint16_t                id;
        struct config_accessor* cfg;
    };

    static inline bool operator<(const domain& lhs, const domain& rhs)
    {
        return lhs.id < rhs.id;
    }
}

#endif /* __KERNEL_PCI_DOMAIN_H */
