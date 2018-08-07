#ifndef __KERNEL_NET_TCP_SOCKET_H
#define __KERNEL_NET_TCP_SOCKET_H

#include "header.h"
#include "net/ip/ip.h"
#include "k++/delegate.h"

namespace net
{
    struct interface;
}

namespace tcp
{
    struct socket;

    struct listener
    {
        // Decide whether or not to accept a SYN connection request.
        kernel::delegate<bool(const header* syn)>   should_accept;

        // Called when a new connection has been accepted.  The client can
        // fill out the rx_readable delegate on the socket if they wish to get
        // rx packet notifications.
        kernel::delegate<void(socket*)>             socket_accepted;
    };
    typedef typeof_field(listener,should_accept)   should_accept_delegate;
    typedef typeof_field(listener,socket_accepted) socket_accepted_delegate;

    struct socket
    {
        enum tcp_state
        {
            TCP_CLOSED,
            TCP_LISTEN,
        };

        net::interface*                 intf;
        tcp::ll_ipv4_tcp_headers        hdrs;
        tcp_state                       state;
        tcp_state                       prev_state;

        // Passive open.
        socket(net::interface* intf, uint16_t port);
    };
}

#endif /* __KERNEL_NET_TCP_SOCKET_H */
