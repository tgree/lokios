#include "../dhcpc.h"
#include "../arp.h"
#include "finterface.h"
#include "../../mock/fschedule.h"
#include "../../mock/fconsole.h"

#define ASSERT_STATE(s) TASSERT(intf.dhcpc->state == dhcp::client::s);
#define SERVER_MAC      eth::addr{0x11,0x12,0x13,0x14,0x15,0x16}
#define SERVER_IP       ipv4::addr{192,168,1,1}
#define CLIENT_MAC      eth::addr{0x01,0x02,0x03,0x04,0x05,0x06}
#define CLIENT_IP       ipv4::addr{192,168,1,100}
#define GW_IP           ipv4::addr{192,168,1,255}
#define SUBNET_MASK     ipv4::addr{255,255,255,0}
#define DNS_IP          ipv4::addr{192,168,1,2}
#define LEASE_TIME      86400

static eth::finterface intf(CLIENT_MAC);

template<typename hw_traits, typename proto_traits>
static void
rx_arp_reply(arp::service<hw_traits,proto_traits>* service,
    const eth::addr& hw_addr, const ipv4::addr& proto_addr)
{
    typedef typeof(*service) arp_service;

    eth::rx_page p;
    memset(p.payload,0,sizeof(p.payload));

    auto* f           = (typename arp_service::arp_frame*)p.payload;
    p.eth_offset      = 0;
    p.eth_len         = sizeof(*f);
    f->hdr.dst_mac    = CLIENT_MAC;
    f->hdr.src_mac    = hw_addr;
    f->hdr.ether_type = 0x0806;
    f->msg.htype      = hw_traits::arp_hw_type;
    f->msg.ptype      = proto_traits::ether_type;
    f->msg.hlen       = sizeof(f->msg.sha);
    f->msg.plen       = sizeof(f->msg.spa);
    f->msg.oper       = 2;
    f->msg.sha        = hw_addr;
    f->msg.spa        = proto_addr;
    f->msg.tha        = CLIENT_MAC;

    intf.arpc_ipv4->handle_rx_frame(&p);
}

static dhcp::eth_message*
prepare_rx_message(eth::rx_page* p)
{
    memset(p->payload,0,sizeof(p->payload));
    p->eth_offset = 0;
    p->eth_len    = sizeof(dhcp::eth_message);

    auto* m             = (dhcp::eth_message*)p->payload;
    m->llhdr.src_mac    = SERVER_MAC;
    m->llhdr.dst_mac    = CLIENT_MAC;
    m->llhdr.ether_type = 0x0800;
    m->iphdr.src_ip     = SERVER_IP;
    m->iphdr.dst_ip     = CLIENT_IP;
    m->uhdr.src_port    = 67;

    return m;
}

static void
rx_offer()
{
    eth::rx_page p;
    auto* m = prepare_rx_message(&p);
    m->msg.format_offer(intf.dhcpc->xid,intf.hw_mac,ipv4::addr{0,0,0,0},
                        CLIENT_IP,SERVER_IP,GW_IP);
    intf.dhcpc->handle_rx_dhcp(&intf,&p);
}

static void
rx_ack()
{
    eth::rx_page p;
    auto* m = prepare_rx_message(&p);
    m->msg.format_ack(intf.dhcpc->xid,intf.hw_mac,ipv4::addr{0,0,0,0},
                      CLIENT_IP,SERVER_IP,GW_IP,SUBNET_MASK,DNS_IP,LEASE_TIME);
    intf.dhcpc->handle_rx_dhcp(&intf,&p);
}

static void
transition_DHCP_INIT()
{
    ASSERT_STATE(DHCP_INIT);
}

static void
transition_DHCP_SELECTING_WAIT_RX_RESP_TX_COMP()
{
    transition_DHCP_INIT();
    texpect("eth::interface::post_tx_frame",want(op,&intf.dhcpc->send_op));
    intf.dhcpc->start();
    ASSERT_STATE(DHCP_SELECTING_WAIT_RX_RESP_TX_COMP);
    // TODO: Validate contents of the send frame buffer.
}

static void
transition_DHCP_SELECTING_WAIT_RX_RESP()
{
    transition_DHCP_SELECTING_WAIT_RX_RESP_TX_COMP();
    intf.dhcpc->handle_tx_send_comp();
    ASSERT_STATE(DHCP_SELECTING_WAIT_RX_RESP);
}

static void
transition_DHCP_SELECTING_WAIT_TX_COMP()
{
    transition_DHCP_SELECTING_WAIT_RX_RESP_TX_COMP();
    rx_offer();
    ASSERT_STATE(DHCP_SELECTING_WAIT_TX_COMP);
}

static void
transition_DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP()
{
    transition_DHCP_SELECTING_WAIT_TX_COMP();
    texpect("eth::interface::post_tx_frame",want(op,&intf.dhcpc->send_op));
    intf.dhcpc->handle_tx_send_comp();
    ASSERT_STATE(DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP);
}

static void
transition_DHCP_REQUESTING_WAIT_RX_RESP()
{
    transition_DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP();
    intf.dhcpc->handle_tx_send_comp();
    ASSERT_STATE(DHCP_REQUESTING_WAIT_RX_RESP);
}

static void
transition_DHCP_REQUESTING_WAIT_TX_COMP()
{
    transition_DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP();
    rx_ack();
    ASSERT_STATE(DHCP_REQUESTING_WAIT_TX_COMP);
}

static void
transition_DHCP_REQUESTING_WAIT_ARP_COMP()
{
    transition_DHCP_REQUESTING_WAIT_TX_COMP();
    texpect("eth::interface::post_tx_frame");
    intf.dhcpc->handle_tx_send_comp();
    ASSERT_STATE(DHCP_REQUESTING_WAIT_ARP_COMP);
}

static void
transition_DHCP_DECLINED_WAIT_TX_COMP()
{
    transition_DHCP_REQUESTING_WAIT_ARP_COMP();

    TASSERT(!intf.arpc_ipv4->arp_lookup_ops.empty());
    auto* op = klist_front(intf.arpc_ipv4->arp_lookup_ops,link);
    auto* cqe = op->cqe;
    intf.arpc_ipv4->lookup_cb(&op->tx_op);
    rx_arp_reply(intf.arpc_ipv4,eth::addr{0x11,0x22,0x33,0x44,0x55,0x66},
                 CLIENT_IP);

    texpect("eth::interface::post_tx_frame");
    kernel::fire_work(cqe);

    ASSERT_STATE(DHCP_DECLINED_WAIT_TX_COMP);
}

static void
transition_DHCP_BOUND_WAIT_TIMEOUT()
{
    transition_DHCP_REQUESTING_WAIT_ARP_COMP();

    for (size_t i=0; i<ARP_RETRY_ATTEMPTS; ++i)
    {
        TASSERT(!intf.arpc_ipv4->arp_lookup_ops.empty());
        auto* op = klist_front(intf.arpc_ipv4->arp_lookup_ops,link);
        auto* cqe = op->cqe;
        intf.arpc_ipv4->lookup_cb(&op->tx_op);

        intf.arpc_ipv4->handle_lookup_timeout(&op->timeout_cqe);

        if (i == ARP_RETRY_ATTEMPTS - 1)
            texpect("eth::interface::handle_dhcp_success");
        else
            texpect("eth::interface::post_tx_frame");
        kernel::fire_work(cqe);
    }

    ASSERT_STATE(DHCP_BOUND_WAIT_TIMEOUT);
}

static void
transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP()
{
    transition_DHCP_BOUND_WAIT_TIMEOUT();
    texpect("eth::interface::post_tx_frame");
    kernel::fire_timer(&intf.dhcpc->t1_wqe);
    ASSERT_STATE(DHCP_RENEWING_WAIT_RX_RESP_TX_COMP);
}

static void
transition_DHCP_RENEWING_WAIT_RX_RESP()
{
    transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP();
    intf.dhcpc->handle_tx_send_comp();
    ASSERT_STATE(DHCP_RENEWING_WAIT_RX_RESP);
}

static void
transition_DHCP_RENEWING_WAIT_TX_COMP()
{
    transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP();
    rx_ack();
    ASSERT_STATE(DHCP_RENEWING_WAIT_TX_COMP);
}

static void
transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED()
{
    transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP();
    kernel::fire_timer(&intf.dhcpc->t2_wqe);
    ASSERT_STATE(DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED);
}

static void
transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED()
{
    transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED();
    kernel::fire_timer(&intf.dhcpc->lease_timer_wqe);
    ASSERT_STATE(DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED);
}

static void
transition_DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED()
{
    transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED();
    rx_ack();
    ASSERT_STATE(DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED);
}

static void
transition_DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED()
{
    transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED();
    rx_ack();
    ASSERT_STATE(DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED);
}

static void
transition_DHCP_REBINDING_WAIT_RX_RESP_TX_COMP()
{
    transition_DHCP_RENEWING_WAIT_RX_RESP();
    texpect("eth::interface::post_tx_frame");
    kernel::fire_timer(&intf.dhcpc->t2_wqe);
    ASSERT_STATE(DHCP_REBINDING_WAIT_RX_RESP_TX_COMP);
}

static void
transition_DHCP_REBINDING_WAIT_RX_RESP()
{
    transition_DHCP_REBINDING_WAIT_RX_RESP_TX_COMP();
    intf.dhcpc->handle_tx_send_comp();
    ASSERT_STATE(DHCP_REBINDING_WAIT_RX_RESP);
}

static void
transition_DHCP_REBINDING_WAIT_TX_COMP()
{
    transition_DHCP_REBINDING_WAIT_RX_RESP_TX_COMP();
    rx_ack();
    ASSERT_STATE(DHCP_REBINDING_WAIT_TX_COMP);
}

static void
transition_DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED()
{
    transition_DHCP_REBINDING_WAIT_RX_RESP_TX_COMP();
    kernel::fire_timer(&intf.dhcpc->lease_timer_wqe);
    ASSERT_STATE(DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED);
}

static void
transition_DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED()
{
    transition_DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED();
    rx_ack();
    ASSERT_STATE(DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED);
}

class tmock_test
{
    TMOCK_TEST(test_DHCP_INIT)
    {
        transition_DHCP_INIT();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_SELECTING_WAIT_RX_RESP_TX_COMP)
    {
        transition_DHCP_SELECTING_WAIT_RX_RESP_TX_COMP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_SELECTING_WAIT_RX_RESP)
    {
        transition_DHCP_SELECTING_WAIT_RX_RESP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_SELECTING_WAIT_TX_COMP)
    {
        transition_DHCP_SELECTING_WAIT_TX_COMP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
        TASSERT(intf.dhcpc->server_mac == SERVER_MAC);
        TASSERT(intf.dhcpc->server_ip == SERVER_IP);
        TASSERT(intf.dhcpc->requested_addr == CLIENT_IP);
    }

    TMOCK_TEST(test_DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP)
    {
        transition_DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_REQUESTING_WAIT_RX_RESP)
    {
        transition_DHCP_REQUESTING_WAIT_RX_RESP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_REQUESTING_WAIT_TX_COMP)
    {
        transition_DHCP_REQUESTING_WAIT_TX_COMP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_REQUESTING_WAIT_ARP_COMP)
    {
        transition_DHCP_REQUESTING_WAIT_ARP_COMP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_DECLINED_WAIT_TX_COMP)
    {
        transition_DHCP_DECLINED_WAIT_TX_COMP();
    }

    TMOCK_TEST(test_DHCP_BOUND_WAIT_TIMEOUT)
    {
        transition_DHCP_BOUND_WAIT_TIMEOUT();
        TASSERT(intf.dhcpc->t1_wqe.is_armed());
        TASSERT(intf.dhcpc->t2_wqe.is_armed());
        TASSERT(intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP)
    {
        transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(intf.dhcpc->t2_wqe.is_armed());
        TASSERT(intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_RENEWING_WAIT_RX_RESP)
    {
        transition_DHCP_RENEWING_WAIT_RX_RESP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(intf.dhcpc->t2_wqe.is_armed());
        TASSERT(intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_RENEWING_WAIT_TX_COMP)
    {
        transition_DHCP_RENEWING_WAIT_TX_COMP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED)
    {
        transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED)
    {
        transition_DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED)
    {
        transition_DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED)
    {
        transition_DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_REBINDING_WAIT_RX_RESP_TX_COMP)
    {
        transition_DHCP_REBINDING_WAIT_RX_RESP_TX_COMP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_REBINDING_WAIT_RX_RESP)
    {
        transition_DHCP_REBINDING_WAIT_RX_RESP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_REBINDING_WAIT_TX_COMP)
    {
        transition_DHCP_REBINDING_WAIT_TX_COMP();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED)
    {
        transition_DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }

    TMOCK_TEST(test_DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED)
    {
        transition_DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED();
        TASSERT(!intf.dhcpc->t1_wqe.is_armed());
        TASSERT(!intf.dhcpc->t2_wqe.is_armed());
        TASSERT(!intf.dhcpc->lease_timer_wqe.is_armed());
        TASSERT(!intf.dhcpc->rx_dropped_timer.is_armed());
    }
};

int
main(int argc, const char* argv[])
{
    kernel::fconsole_suppress_output = true;
    return tmock::run_tests(argc,argv);
}
