#include "dhcpc.h"
#include "kernel/console.h"

using kernel::_kassert;
using kernel::console::printf;

#define DEBUG_TRANSITIONS 0

#if DEBUG_TRANSITIONS
#define TRANSITION(s) \
    do                                                              \
    {                                                               \
        printf("%02X:%02X:%02X:%02X:%02X:%02X: %u: %u -> " #s "\n", \
               intf->hw_mac[0],intf->hw_mac[1],intf->hw_mac[2],     \
               intf->hw_mac[3],intf->hw_mac[4],intf->hw_mac[5],     \
               __LINE__,state);                                     \
        state = (s);                                                \
    } while(0)
#else
#define TRANSITION(s) state = (s)
#endif

static void
handle_dhcp_client_send_comp_bounce(eth::tx_op* op)
{
    auto* c = container_of(op,dhcp::client,send_op);
    c->handle_tx_send_comp();
}

dhcp::client::client(eth::interface* intf):
    state(DHCP_INIT),
    intf(intf),
    xid(0x21324354)
{
    t1_wqe.fn               = timer_delegate(handle_t1_expiry);
    t2_wqe.fn               = timer_delegate(handle_t2_expiry);
    lease_timer_wqe.fn      = timer_delegate(handle_lease_expiry);
    t1_wqe.args[0]          = (uintptr_t)this;
    t2_wqe.args[0]          = (uintptr_t)this;
    lease_timer_wqe.args[0] = (uintptr_t)this;

    packet.llhdr.src_mac          = intf->hw_mac;
    packet.llhdr.ethertype        = 0x0800;
    packet.iphdr.version_ihl      = 0x45;
    packet.iphdr.dscp_ecn         = 0;
    packet.iphdr.total_len        = sizeof(packet) - sizeof(packet.llhdr);
    packet.iphdr.identification   = 0;
    packet.iphdr.flags_fragoffset = 0x4000;
    packet.iphdr.ttl              = 1;
    packet.iphdr.proto            = 0x11;
    packet.uhdr.src_port          = 68;
    packet.uhdr.dst_port          = 67;
    packet.uhdr.len               = sizeof(packet) - sizeof(packet.llhdr) -
                                    sizeof(packet.iphdr);
    packet.uhdr.checksum          = 0; // TODO: Disabled

    send_op.cb            = handle_dhcp_client_send_comp_bounce;
    send_op.nalps         = 1;
    send_op.alps[0].paddr = (kernel::dma_addr64)&packet;
    send_op.alps[0].len   = sizeof(packet);
}

void
dhcp::client::TRANSITION_WAIT_RX_RESP()
{
    // Transition from *_WAIT_RX_RESP_TX_COMP -> *_WAIT_RX_RESP
    switch (state)
    {
        case DHCP_SELECTING_WAIT_RX_RESP_TX_COMP:
            TRANSITION(DHCP_SELECTING_WAIT_RX_RESP);
        break;

        case DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP:
            TRANSITION(DHCP_REQUESTING_WAIT_RX_RESP);
        break;

        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP:
            TRANSITION(DHCP_RENEWING_WAIT_RX_RESP);
        break;

        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP:
            TRANSITION(DHCP_REBINDING_WAIT_RX_RESP);
        break;

        case DHCP_INIT:
        case DHCP_SELECTING_WAIT_RX_RESP:
        case DHCP_SELECTING_WAIT_TX_COMP:
        case DHCP_REQUESTING_WAIT_RX_RESP:
        case DHCP_REQUESTING_WAIT_TX_COMP:
        case DHCP_DECLINED_WAIT_TX_COMP:
        case DHCP_BOUND_WAIT_TIMEOUT:
        case DHCP_RENEWING_WAIT_RX_RESP:
        case DHCP_RENEWING_WAIT_TX_COMP:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_RX_RESP:
        case DHCP_REBINDING_WAIT_TX_COMP:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED:
            kernel::panic("Illegal transition");
        break;
    }
}

void
dhcp::client::TRANSITION_WAIT_TX_COMP()
{
    // Transition from *_WAIT_RX_RESP_TX_COMP -> *_WAIT_TX_COMP
    switch (state)
    {
        case DHCP_SELECTING_WAIT_RX_RESP_TX_COMP:
            TRANSITION(DHCP_SELECTING_WAIT_TX_COMP);
        break;

        case DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP:
            TRANSITION(DHCP_REQUESTING_WAIT_TX_COMP);
        break;

        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP:
            TRANSITION(DHCP_RENEWING_WAIT_TX_COMP);
        break;

        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED:
            TRANSITION(DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED);
        break;

        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
            TRANSITION(DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED);
        break;

        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP:
            TRANSITION(DHCP_REBINDING_WAIT_TX_COMP);
        break;

        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
            TRANSITION(DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED);
        break;

        case DHCP_INIT:
        case DHCP_SELECTING_WAIT_RX_RESP:
        case DHCP_SELECTING_WAIT_TX_COMP:
        case DHCP_REQUESTING_WAIT_RX_RESP:
        case DHCP_REQUESTING_WAIT_TX_COMP:
        case DHCP_DECLINED_WAIT_TX_COMP:
        case DHCP_BOUND_WAIT_TIMEOUT:
        case DHCP_RENEWING_WAIT_RX_RESP:
        case DHCP_RENEWING_WAIT_TX_COMP:
        case DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_RX_RESP:
        case DHCP_REBINDING_WAIT_TX_COMP:
        case DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED:
            kernel::panic("Illegal transition");
        break;
    }
}

void
dhcp::client::start()
{
    start_selecting();
}

void
dhcp::client::start_selecting()
{
    kassert(state == DHCP_INIT);
    kassert(!lease_timer_wqe.is_armed());
    kassert(!t2_wqe.is_armed());
    kassert(!t1_wqe.is_armed());

    packet.llhdr.dst_mac         = eth::broadcast_addr;
    packet.iphdr.src_ip          = ipv4::addr{0,0,0,0};
    packet.iphdr.dst_ip          = ipv4::broadcast_addr;
    packet.iphdr.header_checksum = 0;
    packet.iphdr.header_checksum = ipv4::csum(&packet.iphdr);
    packet.msg.format_discover(++xid,intf->hw_mac);

    TRANSITION(DHCP_SELECTING_WAIT_RX_RESP_TX_COMP);
    intf->post_tx_frame(&send_op);
}

void
dhcp::client::start_requesting()
{
    // We've extracted requested_addr from DHCPOFFER.yiaddr and server_ip from
    // DHCPOFFER.options.  We must send a DHCPREQUEST to select our preferred
    // offer.
    kassert(state == DHCP_SELECTING_WAIT_RX_RESP ||
            state == DHCP_SELECTING_WAIT_TX_COMP);
    kassert(!t1_wqe.is_armed());
    kassert(!t2_wqe.is_armed());
    kassert(!lease_timer_wqe.is_armed());

    packet.llhdr.dst_mac         = eth::broadcast_addr;
    packet.iphdr.src_ip          = ipv4::addr{0,0,0,0};
    packet.iphdr.dst_ip          = ipv4::broadcast_addr;
    packet.iphdr.header_checksum = 0;
    packet.iphdr.header_checksum = ipv4::csum(&packet.iphdr);
    packet.msg.format_request(xid,intf->hw_mac,requested_addr,server_ip);

    TRANSITION(DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP);
    intf->post_tx_frame(&send_op);
}

void
dhcp::client::start_renewing()
{
    // The T1 timer has expired in the bound state.  We need to start renewing
    // the lease.
    kassert(state == DHCP_BOUND_WAIT_TIMEOUT);
    kassert(!t1_wqe.is_armed());
    kassert(t2_wqe.is_armed());
    kassert(lease_timer_wqe.is_armed());

    packet.llhdr.dst_mac         = server_mac;
    packet.iphdr.src_ip          = requested_addr;
    packet.iphdr.dst_ip          = server_ip;
    packet.iphdr.header_checksum = 0;
    packet.iphdr.header_checksum = ipv4::csum(&packet.iphdr);
    packet.msg.format_request(++xid,intf->hw_mac,requested_addr,
                              ipv4::addr{0,0,0,0},requested_addr);

    TRANSITION(DHCP_RENEWING_WAIT_RX_RESP_TX_COMP);
    intf->post_tx_frame(&send_op);
}

void
dhcp::client::start_rebinding()
{
    // The T2 timer has expired in the renewing state.  We need to start
    // rebinding to a different server.
    kassert(state == DHCP_RENEWING_WAIT_RX_RESP ||  
            state == DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED);
    kassert(!t1_wqe.is_armed());
    kassert(!t2_wqe.is_armed());
    kassert(lease_timer_wqe.is_armed());

    packet.llhdr.dst_mac         = eth::broadcast_addr;
    packet.iphdr.src_ip          = ipv4::addr{0,0,0,0};
    packet.iphdr.dst_ip          = ipv4::broadcast_addr;
    packet.iphdr.header_checksum = 0;
    packet.iphdr.header_checksum = ipv4::csum(&packet.iphdr);
    packet.msg.format_request(++xid,intf->hw_mac,requested_addr,
                              ipv4::addr{0,0,0,0},requested_addr);

    TRANSITION(DHCP_REBINDING_WAIT_RX_RESP_TX_COMP);
    intf->post_tx_frame(&send_op);
}

void
dhcp::client::process_request_reply()
{
    kassert(state == DHCP_REQUESTING_WAIT_RX_RESP ||
            state == DHCP_REQUESTING_WAIT_TX_COMP ||
            state == DHCP_RENEWING_WAIT_RX_RESP ||
            state == DHCP_RENEWING_WAIT_TX_COMP ||
            state == DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED ||
            state == DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED ||
            state == DHCP_REBINDING_WAIT_TX_COMP ||
            state == DHCP_REBINDING_WAIT_RX_RESP ||
            state == DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED);
    kassert(!lease_timer_wqe.is_armed());
    kassert(!t2_wqe.is_armed());
    kassert(!t1_wqe.is_armed());

    // We've received a DHCPACK or a DHCPNAK and our DHCPREQUEST send has
    // completed.  If we received a DHCPNAK we will have cleared requested_addr
    // to 0.0.0.0.
    if (requested_addr == ipv4::addr{0,0,0,0})
    {
        TRANSITION(DHCP_INIT);
        intf->handle_dhcp_failure();
        return;
    }

    // Okay, we got an ACK so the server is leasing us this address.
    // TODO: ARP this address to make sure it's unused.
    kernel::cpu::schedule_timer_sec(&lease_timer_wqe,lease);
    kernel::cpu::schedule_timer_sec(&t2_wqe,t2);
    kernel::cpu::schedule_timer_sec(&t1_wqe,2);
    TRANSITION(DHCP_BOUND_WAIT_TIMEOUT);
    intf->handle_dhcp_success();
}

void
dhcp::client::handle_tx_send_comp()
{
    switch (state)
    {
        case DHCP_SELECTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP:
            // TODO: Start response retry timer.
            TRANSITION_WAIT_RX_RESP();
        break;

        case DHCP_SELECTING_WAIT_TX_COMP:
            start_requesting();
        break;

        case DHCP_RENEWING_WAIT_TX_COMP:
            kernel::cpu::cancel_timer(&t2_wqe);
        case DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED:
        case DHCP_REBINDING_WAIT_TX_COMP:
            kernel::cpu::cancel_timer(&lease_timer_wqe);
        case DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED:
        case DHCP_REQUESTING_WAIT_TX_COMP:
            process_request_reply();
        break;

        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED:
            start_rebinding();
        break;

        case DHCP_DECLINED_WAIT_TX_COMP:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
            TRANSITION(DHCP_INIT);
            intf->handle_dhcp_failure();
        break;

        case DHCP_INIT:
        case DHCP_SELECTING_WAIT_RX_RESP:
        case DHCP_REQUESTING_WAIT_RX_RESP:
        case DHCP_BOUND_WAIT_TIMEOUT:
        case DHCP_RENEWING_WAIT_RX_RESP:
        case DHCP_REBINDING_WAIT_RX_RESP:
            kernel::panic("unexpected send completion");
        break;
    }
}

void
dhcp::client::handle_rx_dhcp(eth::rx_page* p) try
{
    auto* resp = (dhcp::eth_message*)(p->payload + p->eth_offset);
    if (*(eth::addr*)resp->msg.chaddr != intf->hw_mac)
        return;
    if (resp->msg.xid != xid)
        return;

    printf("rx msg src ip %u.%u.%u.%u dst ip %u.%u.%u.%u\n",
           resp->iphdr.src_ip[0],resp->iphdr.src_ip[1],
           resp->iphdr.src_ip[2],resp->iphdr.src_ip[3],
           resp->iphdr.dst_ip[0],resp->iphdr.dst_ip[1],
           resp->iphdr.dst_ip[2],resp->iphdr.dst_ip[3]);
    switch (resp->msg.get_option<message_type_option>())
    {
        case dhcp::DHCP_OFFER:  handle_rx_dhcp_offer(resp);     break;
        case dhcp::DHCP_ACK:    handle_rx_dhcp_ack(&resp->msg); break;
        case dhcp::DHCP_NAK:    handle_rx_dhcp_nak(&resp->msg); break;

        case dhcp::DHCP_DISCOVER:
        case dhcp::DHCP_REQUEST:
        case dhcp::DHCP_DECLINE:
        case dhcp::DHCP_RELEASE:
        case dhcp::DHCP_INFORM:
        break;
    }
}
catch (dhcp::option_exception& e)
{
    printf("DHCP rx error with option %u (%s): %s\n",e.tag,e.type,e.c_str());
}

void
dhcp::client::handle_rx_dhcp_offer(const dhcp::eth_message* m)
{
    // TODO: Cancel response retry timer.
    switch (state)
    {
        case DHCP_SELECTING_WAIT_RX_RESP_TX_COMP:
            server_mac     = m->llhdr.src_mac;
            requested_addr = m->msg.yiaddr;
            server_ip      = m->msg.get_option<server_id_option>();
            TRANSITION_WAIT_TX_COMP();
        break;

        case DHCP_SELECTING_WAIT_RX_RESP:
            server_mac     = m->llhdr.src_mac;
            requested_addr = m->msg.yiaddr;
            server_ip      = m->msg.get_option<server_id_option>();
            start_requesting();
        break;

        case DHCP_INIT:
        case DHCP_SELECTING_WAIT_TX_COMP:
        case DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_REQUESTING_WAIT_RX_RESP:
        case DHCP_REQUESTING_WAIT_TX_COMP:
        case DHCP_DECLINED_WAIT_TX_COMP:
        case DHCP_BOUND_WAIT_TIMEOUT:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP:
        case DHCP_RENEWING_WAIT_RX_RESP:
        case DHCP_RENEWING_WAIT_TX_COMP:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP:
        case DHCP_REBINDING_WAIT_RX_RESP:
        case DHCP_REBINDING_WAIT_TX_COMP:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED:
            printf("spurious DHCPOFFER received\n");
        break;
    }
}

void
dhcp::client::handle_rx_dhcp_ack(const dhcp::message* m)
{
    // TODO: Cancel response timer.
    auto sn_ip  = m->get_option<subnet_mask_option>(ipv4::addr{0,0,0,0});
    auto gw_ip  = m->get_option<router_option>(ipv4::addr{0,0,0,0});
    auto dns_ip = m->get_option<dns_option>(ipv4::addr{0,0,0,0});
    switch (state)
    {
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP:
            kernel::cpu::cancel_timer(&t2_wqe);
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP:
            kernel::cpu::cancel_timer(&lease_timer_wqe);
        case DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
            requested_addr = m->yiaddr;
            subnet_mask    = sn_ip;
            gw_addr        = gw_ip;
            dns_addr       = dns_ip;
            lease          = m->get_option<lease_option>();
            t1             = m->get_option<t1_option>(lease/2);
            t2             = m->get_option<t2_option>(7*lease/8);
            TRANSITION_WAIT_TX_COMP();
        break;

        case DHCP_RENEWING_WAIT_RX_RESP:
            kernel::cpu::cancel_timer(&t2_wqe);
        case DHCP_REBINDING_WAIT_RX_RESP:
            kernel::cpu::cancel_timer(&lease_timer_wqe);
        case DHCP_REQUESTING_WAIT_RX_RESP:
            requested_addr = m->yiaddr;
            subnet_mask    = sn_ip;
            gw_addr        = gw_ip;
            dns_addr       = dns_ip;
            lease          = m->get_option<lease_option>();
            t1             = m->get_option<t1_option>(lease/2);
            t2             = m->get_option<t2_option>(7*lease/8);
            process_request_reply();
        break;

        case DHCP_INIT:
        case DHCP_SELECTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_SELECTING_WAIT_RX_RESP:
        case DHCP_SELECTING_WAIT_TX_COMP:
        case DHCP_REQUESTING_WAIT_TX_COMP:
        case DHCP_DECLINED_WAIT_TX_COMP:
        case DHCP_BOUND_WAIT_TIMEOUT:
        case DHCP_RENEWING_WAIT_TX_COMP:
        case DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_TX_COMP:
        case DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED:
            printf("spurious DHCPACK received\n");
        break;
    }
}

void
dhcp::client::handle_rx_dhcp_nak(const dhcp::message* m)
{
    // TODO: Cancel response timer.
    switch (state)
    {
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP:
            kernel::cpu::cancel_timer(&t2_wqe);
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP:
            kernel::cpu::cancel_timer(&lease_timer_wqe);
        case DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
            requested_addr = ipv4::addr{0,0,0,0};
            TRANSITION_WAIT_TX_COMP();
        break;

        case DHCP_RENEWING_WAIT_RX_RESP:
            kernel::cpu::cancel_timer(&t2_wqe);
        case DHCP_REBINDING_WAIT_RX_RESP:
            kernel::cpu::cancel_timer(&lease_timer_wqe);
        case DHCP_REQUESTING_WAIT_RX_RESP:
            requested_addr = ipv4::addr{0,0,0,0};
            process_request_reply();
        break;

        case DHCP_INIT:
        case DHCP_SELECTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_SELECTING_WAIT_RX_RESP:
        case DHCP_SELECTING_WAIT_TX_COMP:
        case DHCP_REQUESTING_WAIT_TX_COMP:
        case DHCP_DECLINED_WAIT_TX_COMP:
        case DHCP_BOUND_WAIT_TIMEOUT:
        case DHCP_RENEWING_WAIT_TX_COMP:
        case DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_TX_COMP:
        case DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED:
            printf("spurious DHCPNAK received\n");
        break;
    }
}

void
dhcp::client::handle_t1_expiry()
{
    switch (state)
    {
        case DHCP_BOUND_WAIT_TIMEOUT:
            start_renewing();
        break;

        case DHCP_INIT:
        case DHCP_SELECTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_SELECTING_WAIT_RX_RESP:
        case DHCP_SELECTING_WAIT_TX_COMP:
        case DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_REQUESTING_WAIT_RX_RESP:
        case DHCP_REQUESTING_WAIT_TX_COMP:
        case DHCP_DECLINED_WAIT_TX_COMP:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP:
        case DHCP_RENEWING_WAIT_RX_RESP:
        case DHCP_RENEWING_WAIT_TX_COMP:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP:
        case DHCP_REBINDING_WAIT_RX_RESP:
        case DHCP_REBINDING_WAIT_TX_COMP:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED:
            kernel::panic("unexpected t1 expiry");
        break;
    }
}

void
dhcp::client::handle_t2_expiry()
{
    switch (state)
    {
        case DHCP_RENEWING_WAIT_RX_RESP:
            start_rebinding();
        break;

        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP:
            TRANSITION(DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED);
        break;

        case DHCP_RENEWING_WAIT_TX_COMP:
            TRANSITION(DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED);
        break;

        case DHCP_INIT:
        case DHCP_SELECTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_SELECTING_WAIT_RX_RESP:
        case DHCP_SELECTING_WAIT_TX_COMP:
        case DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_REQUESTING_WAIT_RX_RESP:
        case DHCP_REQUESTING_WAIT_TX_COMP:
        case DHCP_DECLINED_WAIT_TX_COMP:
        case DHCP_BOUND_WAIT_TIMEOUT:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP:
        case DHCP_REBINDING_WAIT_RX_RESP:
        case DHCP_REBINDING_WAIT_TX_COMP:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED:
            kernel::panic("unexpected t2 expiry");
        break;
    }
}

void
dhcp::client::handle_lease_expiry()
{
    switch (state)
    {
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED:
            TRANSITION(DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED);
        break;

        case DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED:
            TRANSITION(DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED);
        break;

        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP:
            TRANSITION(DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED);
        break;

        case DHCP_REBINDING_WAIT_TX_COMP:
            TRANSITION(DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED);
        break;

        case DHCP_REBINDING_WAIT_RX_RESP:
            TRANSITION(DHCP_INIT);
            intf->handle_dhcp_failure();
        break;

        case DHCP_INIT:
        case DHCP_SELECTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_SELECTING_WAIT_RX_RESP:
        case DHCP_SELECTING_WAIT_TX_COMP:
        case DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP:
        case DHCP_REQUESTING_WAIT_RX_RESP:
        case DHCP_REQUESTING_WAIT_TX_COMP:
        case DHCP_DECLINED_WAIT_TX_COMP:
        case DHCP_BOUND_WAIT_TIMEOUT:
        case DHCP_RENEWING_WAIT_RX_RESP:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP:
        case DHCP_RENEWING_WAIT_TX_COMP:
        case DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED:
        case DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED:
            kernel::panic("unexpected lease expiry");
        break;
    }
}
