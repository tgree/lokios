#include "cmd_sock.h"
#include "interface.h"

net::cmd_sock_listener::cmd_sock_listener(net::interface* intf):
    intf(intf)
{
}

void
net::cmd_sock_listener::listen(uint16_t port)
{
    intf->tcp_listen(port,method_delegate(cmd_socket_accepted));
}

void
net::cmd_sock_listener::cmd_socket_accepted(tcp::socket* s)
{
    intf->intf_dbg("cmd_sock connect from %u.%u.%u.%u:%u accepted\n",
                   s->hdrs.ip.dst_ip[0],
                   s->hdrs.ip.dst_ip[1],
                   s->hdrs.ip.dst_ip[2],
                   s->hdrs.ip.dst_ip[3],
                   (uint16_t)s->hdrs.tcp.dst_port);
}
