#ifndef __KERNEL_NET_E1000_H
#define __KERNEL_NET_E1000_H

#include "pci/pci.h"

namespace e1000
{
    struct driver : public kernel::pci::driver
    {
        virtual uint64_t score(kernel::pci::dev* pd) const;
        virtual kernel::pci::dev* claim(kernel::pci::dev* pd) const;
        virtual void release(kernel::pci::dev* pd) const;
        driver();
    };

    struct dev : public kernel::pci::dev
    {
        dev(const kernel::pci::dev* pd, const e1000::driver* owner);
    };
}

#endif /* __KERNEL_NET_E1000_H */
