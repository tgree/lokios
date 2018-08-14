#include "../cmd_sock.h"
#include <tmock/tmock.h>

net::cmd_sock_listener::cmd_sock_listener(net::interface* intf):
    intf(intf)
{
}

void
net::cmd_sock_listener::listen(uint16_t port)
{
    mock("net::cmd_sock_listener::listen",port);
}

void
net::cmd_sock_listener::cmd_socket_accepted(tcp::socket* s)
{
    mock("net::cmd_sock_listener::cmd_socket_accepted",s);
}

void
net::cmd_sock_listener::socket_established(tcp::socket* s)
{
    mock("net::cmd_sock_listener::socket_established",s);
}

void
net::cmd_sock_listener::socket_readable(tcp::socket* s)
{
    mock("net::cmd_sock_listener::socket_readable",s);
}

void
net::cmd_sock_listener::socket_reset(tcp::socket* s)
{
    mock("net::cmd_sock_listener::socket_reset",s);
}
