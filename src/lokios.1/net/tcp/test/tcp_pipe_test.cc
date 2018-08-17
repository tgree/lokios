#include "net/mock/finterface.h"
#include <tmock/tmock.h>

#define LOCAL_IP0   ipv4::addr{1,1,1,1}
#define LOCAL_IP1   ipv4::addr{2,2,2,2}
#define LISTEN_PORT 3333

struct mock_observer : public tcp::socket_observer
{
    virtual void socket_established(tcp::socket* s)
    {
        mock("mock_observer::socket_established",s);
    }

    virtual void socket_readable(tcp::socket* s)
    {
        mock("mock_observer::socket_readable",s);
    }

    virtual void socket_reset(tcp::socket* s)
    {
        mock("mock_observer::socket_reset",s);
    }
};

static mock_observer observer;

struct mock_listener
{
    void socket_accepted(tcp::socket* s)
    {
        s->observer = &observer;
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

static net::finterface intf0(LOCAL_IP0);
static net::finterface intf1(LOCAL_IP1);
static net::fpipe intf_pipe(&intf0,&intf1);
static tcp::socket* passive_socket;
static tcp::socket* active_socket;

class tmock_test
{
    TMOCK_TEST(test_connect)
    {
        mock_listener ml;
        ml.listen(&intf0,3333);

        texpect("mock_listener::socket_accepted",
                capture(s,(uintptr_t*)&passive_socket));

        // Active socket:
        //  - post SYN
        active_socket = intf1.tcp_connect(intf0.ip_addr,LISTEN_PORT,&observer);

        TASSERT(!passive_socket);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Active socket:
        //  - send comp for SYN -> arm retransmit timer
        // Passive socket:
        //  - rx SYN -> post SYN/ACK
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        TASSERT(active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_SYN_RECVD);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Active socket:
        //  - rx SYN/ACK -> post ACK -> go to ESTABLISHED
        //    -> disarm retransmit timer
        // Passive socket:
        //  - send comp for SYN/ACK -> arm retransmit timer
        texpect("mock_observer::socket_established",want(s,active_socket));
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        TASSERT(passive_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_SYN_RECVD);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_ESTABLISHED);

        // Active socket:
        //  - send comp for ACK
        // Passive socket:
        //  - rx ACK -> go to ESTABLISHED -> disarm retransmit timer
        texpect("mock_observer::socket_established",want(s,passive_socket));
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_ESTABLISHED);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_ESTABLISHED);

        // Queue should be idle.
        tmock::assert_equiv(intf_pipe.process_queues(),0U);
    }
};

TMOCK_MAIN();
