#ifndef __KERNEL_NET_TCP_TCP_H
#define __KERNEL_NET_TCP_TCP_H

#include "header.h"
#include "net/net.h"

#define MAX_TX_OPTIONS_SIZE    16

namespace net
{
    struct interface;
}

namespace tcp
{
    struct send_op;

    struct tx_op : public net::tx_op
    {
        kernel::klink       tcp_link;
        kernel::klink       sop_link;
        send_op*            sop;

        uint8_t             llhdr[16];
        ipv4_tcp_headers    hdrs;
        uint8_t             options[MAX_TX_OPTIONS_SIZE];
    };
    KASSERT(offsetof(tx_op,hdrs) ==
            offsetof(tx_op,llhdr) + sizeof_field(tx_op,llhdr));

    // Packet handling.
    uint64_t handle_rx_ipv4_tcp_frame(net::interface* intf, net::rx_page* p);
}

#endif /* __KERNEL_NET_TCP_TCP_H */
