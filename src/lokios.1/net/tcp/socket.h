#ifndef __KERNEL_NET_TCP_SOCKET_H
#define __KERNEL_NET_TCP_SOCKET_H

#include "header.h"
#include "net/eth/header.h"
#include "net/ip/ip.h"
#include "k++/delegate.h"

namespace eth
{
    struct interface;
}

namespace tcp
{
    struct headers
    {
        eth::header     ll;
        ipv4::header    ip;
        tcp::header     tcp;
    } __PACKED__;

    typedef kernel::delegate<int(const tcp::header* syn)> connection_filter;
    struct listener
    {
        eth::interface*     intf;
        uint16_t            port;
        connection_filter   filter_delegate;
    };
}

#endif /* __KERNEL_NET_TCP_SOCKET_H */
