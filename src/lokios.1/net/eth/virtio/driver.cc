#include "virtio_net.h"
#include "pci/pci.h"
#include "kern/console.h"

using kernel::console::printf;
using kernel::_kassert;

virtio_net::driver virtio_net_driver;

virtio_net::driver::driver():
    kernel::pci::driver("virtio_net")
{
}

uint64_t
virtio_net::driver::score(kernel::pci::dev* pd) const
{
    if (pd->config_read_16(0) != 0x1AF4)
        return 0;
    if (pd->config_read_16(2) == 0x1041)
        return 100;
    if (pd->config_read_16(2) == 0x1000)
        return 50;
    return 0;
}

kernel::pci::dev*
virtio_net::driver::claim(kernel::pci::dev* pd) const
{
    uint16_t vid = pd->config_read_16(0);
    uint16_t did = pd->config_read_16(2);

    printf("virtio_net (%04X:%04X): claiming device %04X:%02X:%02X.%u\n",
           vid,did,pd->domain->id,pd->bus,(pd->devfn >> 3),(pd->devfn & 7));
    return new virtio_net::dev(pd,this);
}

void
virtio_net::driver::release(kernel::pci::dev* pd) const
{
    kernel::panic("virtio_net release not supported yet!\n");
}
