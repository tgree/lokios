#ifndef __KERNEL_NET_VIRTIO_NET_H
#define __KERNEL_NET_VIRTIO_NET_H

#include "pci/pci.h"

namespace virtio_net
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
        dev(const kernel::pci::dev* pd, const virtio_net::driver* owner);
    };
}

#endif /* __KERNEL_NET_VIRTIO_NET_H */
