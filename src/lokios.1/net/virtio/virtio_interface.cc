#include "virtio_interface.h"
#include "virtio_net.h"
#include "kernel/console.h"

#define DUMP_TX_PACKETS 0

virtio_net::interface::interface(virtio_net::dev* vdev):
    eth::interface(vdev->mac,vdev->tq.size,vdev->rq.size),
    vdev(vdev)
{
}

virtio_net::interface::~interface()
{
    kernel::panic("So mean.");
}

void
virtio_net::interface::post_tx_frame(eth::tx_op* op)
{
#if DUMP_TX_PACKETS
    kernel::console::printf("Transmitting packet:\n");
    kernel::console::hexdump((void*)op->alps[0].paddr,op->alps[0].len,0);
#endif
    vdev->post_tx_frame(op);
}

void
virtio_net::interface::post_rx_pages(kernel::klist<eth::rx_page>& pages)
{
    vdev->post_rx_pages(pages);
}
