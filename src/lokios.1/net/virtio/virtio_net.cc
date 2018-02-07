/*
 * Driver for the virtio network device.
 * Reference:
 *  http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.pdf
 *  http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html
 *
 * To make this work in qemu you'll need something like the following options:
 *  -device virtio-net-pci,netdev=net0,disable-legacy=on,disable-modern=off, \
 *          vectors=4
 *  -netdev user,id=net0
 */
#include "virtio_net.h"

virtio_net::dev::dev(const kernel::pci::dev* pd,
    const virtio_net::driver* owner):
        kernel::pci::dev(pd,owner)
{
}
