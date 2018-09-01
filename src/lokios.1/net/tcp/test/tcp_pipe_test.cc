#include "net/mock/finterface.h"
#include "kern/mock/fschedule.h"
#include "kern/mock/fconsole.h"
#include <tmock/tmock.h>

#define LOCAL_IP0   ipv4::addr{1,1,1,1}
#define LOCAL_IP1   ipv4::addr{2,2,2,2}
#define LISTEN_PORT 3333

#define WND_SIZE    32768
#define WND_SHIFT   0

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

    virtual void socket_fin_recvd(tcp::socket* s)
    {
        mock("mock_observer::socket_fin_recvd",s);
    }

    virtual void socket_closed(tcp::socket* s)
    {
        mock("mock_observer::socket_closed",s);
    }

    virtual void socket_reset(tcp::socket* s)
    {
        mock("mock_observer::socket_reset",s);
    }
};

static mock_observer observer;

static tcp::socket* passive_socket;
static tcp::socket* active_socket;

static uint8_t snd_data[16384];
static uint8_t rcv_data[16384];
static size_t rcv_pos;

struct fake_observer : public tcp::socket_observer
{
    virtual void socket_established(tcp::socket* s)
    {
        mock("fake_observer::socket_established",s);
    }

    virtual void socket_readable(tcp::socket* s)
    {
        size_t rem = NELEMS(rcv_data) - rcv_pos;
        size_t count = s->rx_avail_bytes;
        TASSERT(rem >= count);
        s->read(rcv_data + rcv_pos,count);
        rcv_pos += count;
    }

    virtual void socket_fin_recvd(tcp::socket* s)
    {
        mock("fake_observer::socket_fin_recvd",s);
    }

    virtual void socket_closed(tcp::socket* s)
    {
        mock("fake_observer::socket_closed",s);
    }

    virtual void socket_reset(tcp::socket* s)
    {
        mock("fake_observer::socket_reset",s);
    }
};

static fake_observer fobserver;

struct fake_listener
{
    void socket_accepted(tcp::socket* s)
    {
        s->observer    = &fobserver;
        passive_socket = s;
    }

    void listen(net::interface* intf, uint16_t port)
    {
        intf->tcp_listen(port,WND_SIZE,method_delegate(socket_accepted));
    }
};

static net::finterface intf0(LOCAL_IP0);
static net::finterface intf1(LOCAL_IP1);
static net::fpipe intf_pipe(&intf0,&intf1);

static void
send_complete(tcp::send_op* sop)
{
    mock("send_complete",sop);
}

static void
establish_connection(uint16_t port)
{
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
    auto e1 = texpect("mock_observer::socket_established",
                      want(s,active_socket));
    tmock::assert_equiv(intf_pipe.process_queues(),1U);
    tcheckpoint(e1);
    TASSERT(!active_socket->retransmit_wqe.is_armed());
    TASSERT(passive_socket->retransmit_wqe.is_armed());
    tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_SYN_RECVD);
    tmock::assert_equiv(active_socket->state,tcp::socket::TCP_ESTABLISHED);

    // Active socket:
    //  - send comp for ACK
    // Passive socket:
    //  - rx ACK -> go to ESTABLISHED -> disarm retransmit timer
    auto e2 = texpect("fake_observer::socket_established",
                      want(s,passive_socket));
    tmock::assert_equiv(intf_pipe.process_queues(),1U);
    tcheckpoint(e2);
    TASSERT(!active_socket->retransmit_wqe.is_armed());
    TASSERT(!passive_socket->retransmit_wqe.is_armed());
    tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_ESTABLISHED);
    tmock::assert_equiv(active_socket->state,tcp::socket::TCP_ESTABLISHED);

    // Queue should be idle.
    tmock::assert_equiv(intf_pipe.process_queues(),0U);
}

class tmock_test
{
    TMOCK_TEST(test_connect)
    {
        fake_listener ml;
        ml.listen(&intf0,LISTEN_PORT);
        establish_connection(LISTEN_PORT);

        // Generate some random data.
        for (size_t i=0; i<NELEMS(snd_data); ++i)
            snd_data[i] = random();

        // Transmit.
        kernel::dma_alp alps[1] = {{kernel::virt_to_phys(snd_data),
                                    sizeof(snd_data)}};
        auto* sop = active_socket->send(NELEMS(alps),alps,
                                        kernel::func_delegate(send_complete));
        texpect("send_complete",want(sop,sop));
        while (intf_pipe.process_queues())
            ;

        // Validate data.
        tmock::assert_equiv(rcv_pos,sizeof(rcv_data));
        TASSERT(memcmp(snd_data,rcv_data,sizeof(rcv_data)) == 0);
    }

    TMOCK_TEST(test_connect_with_retransmits)
    {
        fake_listener ml;
        ml.listen(&intf0,LISTEN_PORT);

        // Active socket:
        //  - post SYN
        active_socket = intf1.tcp_connect(intf0.ip_addr,LISTEN_PORT,&observer);

        TASSERT(!passive_socket);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Active socket:
        //  - send comp for SYN -> arm retransmit timer
        //  - SYN dropped on tx
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(intf_pipe.drop_queues(),1U);
        TASSERT(active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Active socket:
        //  - post SYN
        //  - retransmit timer cleared
        kernel::fire_timer(&active_socket->retransmit_wqe);
        TASSERT(!active_socket->retransmit_wqe.is_armed());

        // Active socket:
        //  - send comp for SYN -> arm retransmit timer
        // Passive socket:
        //  - rx SYN -> post SYN/ACK
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        TASSERT(active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_SYN_RECVD);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Active socket:
        //  - nothing
        // Passive socket:
        //  - send comp for SYN/ACK -> arm retransmit timer
        //  - SYN/ACK dropped on tx
        tmock::assert_equiv(intf_pipe.drop_queues(),1U);
        TASSERT(active_socket->retransmit_wqe.is_armed());
        TASSERT(passive_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_SYN_RECVD);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Active socket:
        //  - post SYN
        //  - retransmit timer cleared
        // Passive socket:
        //  - post SYN/ACK
        //  - retransmit timer cleared
        // Invoke both retransmit timers.  Post both the SYN and the SYN/ACK.
        kernel::fire_timer(&active_socket->retransmit_wqe);
        kernel::fire_timer(&passive_socket->retransmit_wqe);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket->retransmit_wqe.is_armed());

        // Active socket:
        //  - send comp for SYN -> arm retransmit timer
        //  - rx SYN/ACK -> post ACK -> go to ESTABLISHED
        //    -> disarm retransmit timer
        // Passive socket:
        //  - send comp for SYN/ACK -> arm retransmit timer
        //  - rx dup SYN -> post an ACK
        auto e1 = texpect("mock_observer::socket_established",
                         want(s,active_socket));
        tmock::assert_equiv(intf_pipe.process_queues(),2U);
        tcheckpoint(e1);
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_SYN_RECVD);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_ESTABLISHED);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        TASSERT(passive_socket->retransmit_wqe.is_armed());

        // Active socket:
        //  - send comp for ACK
        //  - rx ACK -> do nothing
        // Passive socket:
        //  - send comp for ACK
        //  - rx ACK -> go to ESTABLISHED -> disarm retransmit timer
        auto e2 = texpect("fake_observer::socket_established",
                          want(s,passive_socket));
        tmock::assert_equiv(intf_pipe.process_queues(),2U);
        tcheckpoint(e2);
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_ESTABLISHED);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_ESTABLISHED);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket->retransmit_wqe.is_armed());

        // Generate some random data.
        for (size_t i=0; i<NELEMS(snd_data); ++i)
            snd_data[i] = random();

        // Queue.
        kernel::dma_alp alps[1] = {{kernel::virt_to_phys(snd_data),
                                    sizeof(snd_data)}};
        auto* sop = active_socket->send(NELEMS(alps),alps,
                                        kernel::func_delegate(send_complete));

        // Drop everything, trigger the retransmit timer.
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket->retransmit_wqe.is_armed());
        TASSERT(intf_pipe.drop_queues() > 0);
        tmock::assert_equiv(intf_pipe.process_queues(),0U);
        TASSERT(active_socket->retransmit_wqe.is_armed());
        kernel::fire_timer(&active_socket->retransmit_wqe);
        
        // No more drops.
        texpect("send_complete",want(sop,sop));
        while (intf_pipe.process_queues())
            ;

        // Validate data.
        tmock::assert_equiv(rcv_pos,sizeof(rcv_data));
        TASSERT(memcmp(snd_data,rcv_data,sizeof(rcv_data)) == 0);
    }

    TMOCK_TEST(test_connect_no_listener)
    {
        // Active socket:
        //  - post SYN
        active_socket = intf1.tcp_connect(intf0.ip_addr,LISTEN_PORT,&observer);

        TASSERT(!passive_socket);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Active socket:
        //  - send comp for SYN -> arm retransmit timer
        // Passive socket:
        //  - rx SYN -> post RST/ACK
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        TASSERT(active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Active socket:
        //  - rx RST/ACK -> socket_reset -> go to CLOSED
        //    -> disarm retransmit timer
        texpect("mock_observer::socket_reset",want(s,active_socket));
        auto e = texpect("mock_observer::socket_closed",want(s,active_socket));
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        tcheckpoint(e);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_CLOSED);
        intf1.tcp_delete(active_socket);

        // Queue should be idle.
        tmock::assert_equiv(intf_pipe.process_queues(),0U);
    }

    TMOCK_TEST(test_close)
    {
        fake_listener ml;
        ml.listen(&intf0,LISTEN_PORT);
        establish_connection(LISTEN_PORT);

        // Active socket:
        //  - post FIN -> go to TCP_FIN_WAIT_1
        active_socket->close_send();
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_FIN_WAIT_1);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket->retransmit_wqe.is_armed());

        // Active socket:
        //  - send comp for FIN -> arm retransmit timer
        // Passive socket:
        // - rx FIN -> post ACK -> go to CLOSE_WAIT
        auto e1 = texpect("fake_observer::socket_fin_recvd",
                          want(s,passive_socket));
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        tcheckpoint(e1);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_FIN_WAIT_1);
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_CLOSE_WAIT);
        TASSERT(active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket->retransmit_wqe.is_armed());

        // Active socket:
        //  - rx ACK -> go to FIN_WAIT_2 -> disarm retransmit timer
        // Passive socket:
        //  - send comp for ACK
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_FIN_WAIT_2);
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_CLOSE_WAIT);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket->retransmit_wqe.is_armed());

        // Passive socket:
        //  - post FIN -> go to LAST_ACK
        passive_socket->close_send();
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_FIN_WAIT_2);
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_LAST_ACK);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        TASSERT(!passive_socket->retransmit_wqe.is_armed());

        // Active socket:
        //  - rx FIN -> post ACK -> go to TIME_WAIT
        // Passive socket:
        //  - send comp for FIN -> arm retransmit timer
        auto e2 = texpect("mock_observer::socket_fin_recvd",
                          want(s,active_socket));
        TASSERT(!active_socket->time_wait_wqe.is_armed());
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        tcheckpoint(e2);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_TIME_WAIT);
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_LAST_ACK);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        TASSERT(active_socket->time_wait_wqe.is_armed());
        TASSERT(passive_socket->retransmit_wqe.is_armed());

        // Active socket:
        //  - send comp for ACK
        // Passive socket:
        //  - rx ACK -> go to CLOSED -> disarm retransmit timer
        auto e3 = texpect("fake_observer::socket_closed",
                          want(s,passive_socket));
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        tcheckpoint(e3);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_TIME_WAIT);
        tmock::assert_equiv(passive_socket->state,tcp::socket::TCP_CLOSED);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        TASSERT(active_socket->time_wait_wqe.is_armed());
        TASSERT(!passive_socket->retransmit_wqe.is_armed());
        intf0.tcp_delete(passive_socket);

        // Active socket:
        //  - TIME_WAIT expiry -> closed
        auto e4 = texpect("mock_observer::socket_closed",want(s,active_socket));
        kernel::fire_timer(&active_socket->time_wait_wqe);
        tcheckpoint(e4);
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_CLOSED);
        intf1.tcp_delete(active_socket);
    }
};

int
main(int argc, const char* argv[])
{
    kernel::fconsole_suppress_output = true;
    return tmock::run_tests(argc,argv);
}
