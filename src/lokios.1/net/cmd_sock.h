#ifndef __KERNEL_NET_CMD_SOCK_H
#define __KERNEL_NET_CMD_SOCK_H

#include "tcp/socket.h"

namespace net
{
    struct interface;

    struct cmd_sock_listener
    {
        net::interface* intf;
        kernel::slab    connection_slab;

        void    listen(uint16_t port);
        void    cmd_socket_accepted(tcp::socket* s);

        cmd_sock_listener(net::interface* intf);
    };
}

#endif /* __KERNEL_NET_CMD_SOCK_H */
