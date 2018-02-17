#ifndef __KERNEL_NET_VIRTIO_INTERFACE_H
#define __KERNEL_NET_VIRTIO_INTERFACE_H

#include "../eth.h"

namespace virtio_net
{
    struct dev;

    struct interface : public eth::interface
    {
        virtio_net::dev*    vdev;

        virtual void    post_tx_frame(eth::tx_op* op);
        virtual void    post_rx_pages(kernel::klist<eth::rx_page>& pages);

        interface(virtio_net::dev* vdev);
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_VIRTIO_INTERFACE_H */