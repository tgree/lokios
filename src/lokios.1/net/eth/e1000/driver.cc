#include "e1000.h"
#include "pci/pci.h"
#include "kern/console.h"

using kernel::console::printf;
using kernel::_kassert;

e1000::driver e1000_driver;

e1000::driver::driver():
    kernel::pci::driver("e1000")
{
}

uint64_t
e1000::driver::score(kernel::pci::dev* pd) const
{
    if (pd->config_read_16(0) == 0x8086 &&
        pd->config_read_16(2) == 0x100E)
    {
        return 100;
    }
    return 0;
}

kernel::pci::dev*
e1000::driver::claim(kernel::pci::dev* pd) const
{
    kassert(pd->config_read_16(0) == 0x8086 &&
            pd->config_read_16(2) == 0x100E);
    printf("e1000: claiming device %04X:%02X:%02X.%u\n",
           pd->domain->id,pd->bus,(pd->devfn >> 3),(pd->devfn & 7));
    return new e1000::dev(pd,this);
}

void
e1000::driver::release(kernel::pci::dev* pd) const
{
    kernel::panic("E1000 release not supported yet!\n");
}
