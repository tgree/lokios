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
    struct tx_op : public net::tx_op
    {
        ll_ipv4_tcp_headers hdrs;
        uint8_t             options[MAX_TX_OPTIONS_SIZE];
    };

    // Packet handling.
    uint64_t handle_rx_ipv4_tcp_frame(net::interface* intf, net::rx_page* p);
}

#endif /* __KERNEL_NET_TCP_TCP_H */
