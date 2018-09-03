#include "pci.h"
#include "mm/slab.h"

static kernel::slab dev_slab(sizeof(kernel::pci::dev));

struct _pci_unclaimed_driver : public kernel::pci::driver
{
    virtual uint64_t score(kernel::pci::dev* pd) const
    {
        return 1;
    }

    virtual kernel::pci::dev* claim(kernel::pci::dev* pd) const
    {
        return dev_slab.alloc<kernel::pci::dev>(pd,this);
    }

    virtual void release(kernel::pci::dev* pd) const
    {
        kernel::panic("pci release not supported yet");
    }

    using kernel::pci::driver::driver;
};

_pci_unclaimed_driver pci_unclaimed_driver("unclaimed");
