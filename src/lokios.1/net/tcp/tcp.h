#ifndef __KERNEL_NET_TCP_TCP_H
#define __KERNEL_NET_TCP_TCP_H

#include "net/net.h"

namespace net
{
    struct interface;
}

namespace tcp
{
    // Packet handling.
    uint64_t handle_rx_ipv4_tcp_frame(net::interface* intf, net::rx_page* p);
}

#endif /* __KERNEL_NET_TCP_TCP_H */
