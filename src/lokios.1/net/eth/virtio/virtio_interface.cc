#include "virtio_interface.h"
#include "virtio_net.h"
#include "config.h"
#include "net/eth/checksum.h"
#include "kern/console.h"

virtio_net::interface::interface(virtio_net::dev* vdev):
    eth::interface(vdev->mac,vdev->tq.size,vdev->rq.size,VIRTIO_NET_TX_MTU,
                   VIRTIO_NET_RX_MTU),
    vdev(vdev)
{
}

virtio_net::interface::~interface()
{
    kernel::panic("So mean.");
}

void
virtio_net::interface::issue_phy_read_16(uint8_t offset,
    kernel::work_entry* cqe)
{
    kernel::panic("virtio_net doesn't implement phy_read_16!");
}

void
virtio_net::interface::issue_phy_write_16(uint16_t v, uint8_t offset,
    kernel::work_entry* cqe)
{
    kernel::panic("virtio_net doesn't implement phy_write_16!");
}

void
virtio_net::interface::post_tx_frame(net::tx_op* op)
{
#if VIRTIO_NET_DUMP_TX_PACKETS
    kernel::console::printf("Transmitting packet:\n");
    size_t offset = 0;
    for (size_t i=0; i<op->nalps; ++i)
    {
        kernel::console::hexdump(kernel::phys_to_virt(op->alps[i].paddr),
                                 op->alps[i].len,offset);
        offset += op->alps[i].len;
    }
#endif
    eth::insert_checksums(op);
    vdev->post_tx_frame(op);
}

void
virtio_net::interface::post_rx_pages(kernel::klist<net::rx_page>& pages)
{
    vdev->post_rx_pages(pages);
}
