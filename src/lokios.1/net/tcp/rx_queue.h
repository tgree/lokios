#ifndef __KERNEL_TCP_RX_QUEUE_H
#define __KERNEL_TCP_RX_QUEUE_H

#include "net/net.h"

namespace tcp
{
    struct rx_queue
    {
        kernel::klist<net::rx_page>         pages;
        size_t                              avail_bytes;
        kernel::delegate<void(rx_queue*)>   rq_ready_delegate;

        void    append(net::rx_page* p);
        void    read(void* dst, uint32_t len);

        rx_queue(kernel::delegate<void(rx_queue*)> rq_ready_delegate);
    };
}

#endif /* __KERNEL_TCP_RX_QUEUE_H */
