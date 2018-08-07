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

    typedef kernel::delegate<int(const tcp::header* syn)> connection_filter;
    struct listener
    {
        net::interface*     intf;
        uint16_t            port;
        connection_filter   filter_delegate;
    };
}

#endif /* __KERNEL_NET_TCP_SOCKET_H */
