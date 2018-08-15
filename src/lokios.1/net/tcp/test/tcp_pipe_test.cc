#include "net/mock/finterface.h"
#include <tmock/tmock.h>

#define LOCAL_IP0   ipv4::addr{1,1,1,1}
#define LOCAL_IP1   ipv4::addr{2,2,2,2}
#define LISTEN_PORT 3333

static net::finterface* intf0 = new net::finterface(LOCAL_IP0);
static net::finterface* intf1 = new net::finterface(LOCAL_IP1);
static net::fpipe intf_pipe(intf0,intf1);
tcp::socket* passive_socket;
tcp::socket* active_socket;

struct mock_listener
{
    void socket_accepted(tcp::socket* s)
    {
        mock("mock_listener::socket_accepted",s);
    }

    void socket_readable(tcp::socket* s)
    {
        mock("mock_listener::socket_readable",s);
    }

    void listen(net::interface* intf, uint16_t port)
    {
        intf->tcp_listen(port,method_delegate(socket_accepted));
    }
};

static void
mock_socket_connected(tcp::socket* s)
{
    mock("mock_socket_connected",s);
}

class tmock_test
{
    TMOCK_TEST(test_connect)
    {
        mock_listener ml;
        ml.listen(intf0,3333);

        texpect("mock_listener::socket_accepted",
                capture(s,(uintptr_t*)&passive_socket));

        active_socket = intf1->tcp_connect(intf0->ip_addr,LISTEN_PORT,
                              kernel::func_delegate(mock_socket_connected));

        TASSERT(!passive_socket);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Handle the SYN and post the SYN/ACK.
        tmock::assert_equiv(intf_pipe.process_queues(),1U);

        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_SYN_RECVD);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Drop the SYN/ACK since we don't handle it yet.
        tmock::assert_equiv(intf_pipe.process_queues(),1U);

        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_SYN_RECVD);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Should be no more packets.
        tmock::assert_equiv(intf_pipe.process_queues(),0U);
    }
};

TMOCK_MAIN();
