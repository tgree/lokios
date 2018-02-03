/*
 * Driver for the virtio network device.
 * Reference:
 *  http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.pdf
 *  http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html
 */
#include "virtio_net.h"

virtio_net::dev::dev(const kernel::pci::dev* pd,
    const virtio_net::driver* owner):
        kernel::pci::dev(pd,owner)
{
}
