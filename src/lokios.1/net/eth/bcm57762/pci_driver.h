#ifndef __KERNEL_NET_BCM57762_PCI_DRIVER_H
#define __KERNEL_NET_BCM57762_PCI_DRIVER_H

#include "pci/pci.h"

namespace bcm57762
{
    struct driver : public kernel::pci::driver
    {
        virtual uint64_t score(kernel::pci::dev* pd) const;
        virtual kernel::pci::dev* claim(kernel::pci::dev* pd) const;
        virtual void release(kernel::pci::dev* pd) const;
        driver();
    };
}

#endif /* __KERNEL_NET_BCM57762_PCI_DRIVER_H */
