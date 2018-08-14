#include "../socket.h"
#include "../traits.h"
#include "../tcp.h"
#include "net/mock/finterface.h"
#include "k++/random.h"
#include <tmock/tmock.h>

#define LOCAL_IP    ipv4::addr{1,1,1,1}
#define LOCAL_PORT  10000U
#define REMOTE_IP   ipv4::addr{2,2,2,2}
#define REMOTE_PORT 55555U
#define REMOTE_ISS  1234U
#define REMOTE_WS   4096U

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

static uint32_t
rx_syn()
{
    intf.refill_rx_pages();
    auto* p       = intf.pop_rx_page();
    p->pay_offset = 0;
    p->pay_len    = sizeof(ipv4::header) + sizeof(tcp::header);

    auto* h = p->payload_cast<tcp::ipv4_tcp_headers*>();
    h->ip.version_ihl      = 0x45;
    h->ip.dscp_ecn         = 0;
    h->ip.total_len        = p->pay_len;
    h->ip.identification   = kernel::random();
    h->ip.flags_fragoffset = 0x4000;
    h->ip.ttl              = 64;
    h->ip.proto            = tcp::net_traits::ip_proto;
    h->ip.header_checksum  = 0;
    h->ip.src_ip           = REMOTE_IP;
    h->ip.dst_ip           = intf.ip_addr;
    h->tcp.src_port        = REMOTE_PORT;
    h->tcp.dst_port        = LOCAL_PORT;
    h->tcp.seq_num         = REMOTE_ISS;
    h->tcp.ack_num         = 0;
    h->tcp.flags_offset    = 0x5000;
    h->tcp.window_size     = REMOTE_WS;
    h->tcp.checksum        = 0;
    h->tcp.urgent_pointer  = 0;
    h->tcp.syn             = 1;

    return intf.handle_rx_page(p);
}

class tmock_test
{
    TMOCK_TEST(test_no_listener_connect)
    {
        tmock::assert_equiv(rx_syn(),0U);

        // We should send:
        //  <SEQ=0><ACK=SEG.SEQ+SEG.LEN><CTL=RST,ACK>
        auto* op = static_cast<tcp::tx_op*>(intf.pop_tx_op());
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
        tmock::assert_equiv((uint32_t)op->hdrs.tcp.seq_num,0U);
        tmock::assert_equiv((uint32_t)op->hdrs.tcp.ack_num,REMOTE_ISS+1);
        tmock::assert_equiv(op->hdrs.tcp.window_size,0U);
        tmock::assert_equiv(op->hdrs.tcp.urgent_pointer,0U);
        tmock::assert_equiv((op->hdrs.tcp.flags_offset & 0x0FFF),0x0014U);
        tmock::assert_equiv(op->hdrs.tcp.offset,5U);
        intf.handle_tx_completion(op);
    }
};

TMOCK_MAIN();
