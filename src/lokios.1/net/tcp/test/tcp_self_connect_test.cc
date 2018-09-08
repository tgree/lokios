#include "net/mock/finterface.h"
#include "kern/mock/fschedule.h"
#include "kern/mock/fconsole.h"
#include <tmock/tmock.h>

#define LOCAL_IP0   ipv4::addr{1,1,1,1}
#define LOCAL_IP1   ipv4::addr{2,2,2,2}
#define LISTEN_PORT 3333

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

    virtual void socket_recv_closed(tcp::socket* s)
    {
        mock("fake_observer::socket_recv_closed",s);
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

static net::finterface intf0(LOCAL_IP0);
static net::fpipe intf_pipe(&intf0,&intf0);

static void
send_complete(tcp::send_op* sop)
{
    mock("send_complete",sop);
}

class tmock_test
{
    TMOCK_TEST(test_self_connect)
    {
        // Active socket:
        //  - post SYN
        active_socket = intf0.tcp_connect(intf0.ip_addr,LISTEN_PORT,&fobserver,
                                          LISTEN_PORT);

        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_SYN_SENT);

        // Active socket:
        //  - send comp for SYN -> arm retransmit timer
        //  - rx SYN -> post ACK
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        TASSERT(active_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(active_socket->state,
                            tcp::socket::TCP_SYN_SENT_SYN_RECVD_WAIT_ACK);

        // Active socket:
        //  - rx ACK -> go to ESTABLISHED -> disarm retransmit timer
        texpect("fake_observer::socket_established",want(s,active_socket));
        tmock::assert_equiv(intf_pipe.process_queues(),1U);
        TASSERT(!active_socket->retransmit_wqe.is_armed());
        tmock::assert_equiv(active_socket->state,tcp::socket::TCP_ESTABLISHED);

        // Queue should be idle.
        tmock::assert_equiv(intf_pipe.process_queues(),0U);

        // Generate some random data.
        for (size_t i=0; i<NELEMS(snd_data); ++i)
            snd_data[i] = random();

        // Transmit.
        kernel::dma_alp alps[1] = {{kernel::virt_to_phys(snd_data),
                                    sizeof(snd_data)}};
        auto* sop = active_socket->send(NELEMS(alps),alps,
                                        func_delegate(send_complete));
        texpect("send_complete",want(sop,sop));
        while (intf_pipe.process_queues())
            ;

        // Validate data.
        tmock::assert_equiv(rcv_pos,sizeof(rcv_data));
        TASSERT(memcmp(snd_data,rcv_data,sizeof(rcv_data)) == 0);
    }
};

int
main(int argc, const char* argv[])
{
    kernel::fconsole_suppress_output = true;
    return tmock::run_tests(argc,argv);
}
