#ifndef __KERNEL_NET_CMD_SOCK_H
#define __KERNEL_NET_CMD_SOCK_H

#include "tcp/socket.h"

namespace net
{
    struct interface;

    struct cmd_sock_listener : public tcp::socket_observer
    {
        net::interface* intf;

        void    listen(uint16_t port);
        void    cmd_socket_accepted(tcp::socket* s);

        virtual void    socket_established(tcp::socket* s);
        virtual void    socket_readable(tcp::socket* s);
        virtual void    socket_reset(tcp::socket* s);

        void    send_complete(tcp::send_op* sop);

        void    handle_cmd_arp();
        void    handle_cmd_mem();
        void    handle_cmd_panic();
        void    handle_cmd_exit();
        void    handle_cmd_segv();

        cmd_sock_listener(net::interface* intf);
    };
}

#endif /* __KERNEL_NET_CMD_SOCK_H */
