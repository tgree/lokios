#include "pci_driver.h"
#include "bcm57762.h"
#include "kern/console.h"

using kernel::console::printf;
using kernel::_kassert;

bcm57762::driver bcm57762_driver;

bcm57762::driver::driver():
    kernel::pci::driver("bcm57762")
{
}

uint64_t
bcm57762::driver::score(kernel::pci::dev* pd) const
{
    if (pd->config_read_16(0) == 0x14E4 &&
        pd->config_read_16(2) == 0x1682)
    {
        return 100;
    }
    return 0;
}

kernel::pci::dev*
bcm57762::driver::claim(kernel::pci::dev* pd) const
{
    uint16_t vid = pd->config_read_16(0);
    uint16_t did = pd->config_read_16(2);
    kassert(vid == 0x14E4 && did == 0x1682);

    printf("bcm57762 (%04X:%04X): claiming device %04X:%02X:%02X.%u\n",
           vid,did,pd->domain->id,pd->bus,(pd->devfn >> 3),(pd->devfn & 7));
    return new bcm57762::dev(pd,this);
}

void
bcm57762::driver::release(kernel::pci::dev* pd) const
{
    kernel::panic("bcm57762 release not supported yet!\n");
}
