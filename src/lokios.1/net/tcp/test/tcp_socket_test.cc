#include "../socket.h"
#include "../traits.h"
#include "net/mock/finterface.h"
#include "k++/random.h"
#include <tmock/tmock.h>

#define LOCAL_IP    ipv4::addr{1,1,1,1}
#define LOCAL_PORT  10000U
#define REMOTE_IP   ipv4::addr{2,2,2,2}
#define REMOTE_PORT 55555U
#define REMOTE_ISS  1234U
#define REMOTE_WS   4096U

using namespace tcp;

static net::finterface intf(LOCAL_IP);

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

    void listen(uint16_t port)
    {
        intf.tcp_listen(port,method_delegate(socket_accepted));
    }
};

template<typename... Args>
static uint32_t
rx_packet(Args ...args)
{
    auto* p       = intf.pop_rx_page();
    p->pay_offset = 0;
    p->pay_len    = sizeof(ipv4::header) + sizeof(tcp::header);

    p->payload_cast<tcp::ipv4_tcp_headers*>()->init(SIP{REMOTE_IP},
                                                    DIP{intf.ip_addr},
                                                    SPORT{REMOTE_PORT},
                                                    DPORT{LOCAL_PORT},args...);
    return intf.handle_rx_page(p);
}

static uint32_t
rx_syn()
{
    return rx_packet(SEQ{REMOTE_ISS},CTL{FSYN},WS{REMOTE_WS,0});
}

static void
validate_tx_op(const tcp::tx_op* op, uint32_t seq_num, uint32_t ack_num,
    uint16_t window_size, uint16_t flags, uint8_t offset = 5)
{
    tmock::assert_equiv(op->hdrs.ip.version_ihl,0x45U);
    tmock::assert_equiv(op->hdrs.ip.dscp_ecn,0U);
    tmock::assert_equiv((size_t)op->hdrs.ip.total_len,
                        sizeof(ipv4::header) + op->hdrs.tcp.offset*4);
    tmock::assert_equiv(op->hdrs.ip.flags_fragoffset,0x4000U);
    tmock::assert_equiv(op->hdrs.ip.ttl,64U);
    tmock::assert_equiv(op->hdrs.ip.proto,tcp::net_traits::ip_proto);
    tmock::assert_equiv(op->hdrs.ip.src_ip,LOCAL_IP);
    tmock::assert_equiv(op->hdrs.ip.dst_ip,REMOTE_IP);
    tmock::assert_equiv(op->hdrs.tcp.src_port,LOCAL_PORT);
    tmock::assert_equiv(op->hdrs.tcp.dst_port,REMOTE_PORT);
    tmock::assert_equiv((uint32_t)op->hdrs.tcp.seq_num,seq_num);
    tmock::assert_equiv((uint32_t)op->hdrs.tcp.ack_num,ack_num);
    tmock::assert_equiv(op->hdrs.tcp.window_size,window_size);
    tmock::assert_equiv(op->hdrs.tcp.urgent_pointer,0U);
    tmock::assert_equiv((uint16_t)(op->hdrs.tcp.flags_offset & 0x0FFF),flags);
    tmock::assert_equiv((uint8_t)op->hdrs.tcp.offset,offset);
}

class tmock_test
{
    TMOCK_TEST(test_not_my_ip)
    {
        tmock::assert_equiv(rx_packet(DIP{0,0,0,0}),0U);

        // We should drop the packet.
        TASSERT(intf.posted_ops.empty());
    }

    TMOCK_TEST(test_broadcast_ip)
    {
        tmock::assert_equiv(rx_packet(DIP{ipv4::broadcast_addr}),0U);

        // We should drop the packet.
        TASSERT(intf.posted_ops.empty());
    }

    TMOCK_TEST(test_no_listener_connect)
    {
        tmock::assert_equiv(rx_syn(),0U);

        // We should send:
        //  <SEQ=0><ACK=SEG.SEQ+SEG.LEN><CTL=RST,ACK>
        auto* op = static_cast<tcp::tx_op*>(intf.pop_tx_op());
        validate_tx_op(op,0,REMOTE_ISS+1,0,FRST|FACK);
        intf.handle_tx_completion(op);
    }

    TMOCK_TEST(test_no_listener_ack)
    {
        tmock::assert_equiv(rx_packet(SEQ{12345},ACK{67890},CTL{FACK}),0U);

        // We should send:
        //  <SEQ=SEG.ACK><CTL=RST>
        auto* op = static_cast<tcp::tx_op*>(intf.pop_tx_op());
        validate_tx_op(op,67890,0,0,FRST);
        intf.handle_tx_completion(op);
    }

    TMOCK_TEST(test_passive_connect)
    {
        mock_listener ml;
        ml.listen(LOCAL_PORT);

        tcp::socket* s = NULL;
        texpect("mock_listener::socket_accepted",capture(s,(uintptr_t*)&s));
        tmock::assert_equiv(rx_syn(),0U);

        // We should set:
        //  RCV.NXT = SEG.SEQ+1
        //  IRS     = SEG.SEQ
        tmock::assert_equiv(s->state,tcp::socket::TCP_SYN_RECVD);
        tmock::assert_equiv(s->snd_una,s->iss);
        tmock::assert_equiv(s->snd_nxt,s->iss+1U);
        tmock::assert_equiv(s->snd_wnd,REMOTE_WS);
        tmock::assert_equiv(s->snd_wnd_shift,0);
        tmock::assert_equiv(s->rcv_nxt,REMOTE_ISS+1);
        tmock::assert_equiv(s->rcv_wnd,MAX_RX_WINDOW);
        tmock::assert_equiv(s->rcv_wnd_shift,0);
        tmock::assert_equiv(s->rcv_mss,1460U);
        tmock::assert_equiv(s->irs,REMOTE_ISS);
        tmock::assert_equiv(s->remote_ip,REMOTE_IP);
        tmock::assert_equiv(s->local_port,LOCAL_PORT);
        tmock::assert_equiv(s->remote_port,REMOTE_PORT);

        // We should send:
        //  <SEQ=ISS><ACK=RCV.NXT><CTL=SYN,ACK>
        auto* op = static_cast<tcp::tx_op*>(intf.pop_tx_op());
        validate_tx_op(op,s->iss,REMOTE_ISS+1,0xFFFF,FSYN|FACK,6);

        uint8_t* opt = op->hdrs.tcp.options;
        tmock::assert_equiv(opt[0],2U);
        tmock::assert_equiv(opt[1],4U);
        tmock::assert_equiv(*(be_uint16_t*)(opt + 2),1460U);
        intf.handle_tx_completion(op);
    }
};

TMOCK_MAIN();
