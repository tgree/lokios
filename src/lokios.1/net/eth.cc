#include "eth.h"
#include "dhcp.h"
#include "kernel/console.h"

#define DUMP_GOOD_PACKETS 0
#if DUMP_GOOD_PACKETS
#define dump_good_page(p) dump_rx_page(p)
#else
#define dump_good_page(p)
#endif

using kernel::console::printf;
using kernel::_kassert;

static void
handle_dhcp_send_comp_bounce(eth::tx_op* op)
{
    auto* intf = container_of(op,eth::interface,dhcp_op);
    intf->handle_dhcp_send_comp();
}

static void
dump_rx_page(eth::rx_page* p)
{
    auto* h = (eth::header*)(p->payload + p->eth_offset);
    kernel::console::hexdump(h,p->eth_len,0);
}

eth::interface::interface(const eth::addr& hw_mac, size_t tx_qlen,
    size_t rx_qlen):
        state(EI_STATE_WAIT_ACTIVATE),
        hw_mac(hw_mac),
        ip_addr{0,0,0,0},
        tx_qlen(tx_qlen),
        rx_qlen(rx_qlen)
{
    dhcp_xid          = 0x1A2B3C4D;
    timer_wqe.fn      = work_delegate(activate);
    timer_wqe.args[0] = (uintptr_t)this;
    kernel::cpu::schedule_local_work(&timer_wqe);
}

eth::interface::~interface()
{
}

void
eth::interface::activate()
{
    // Post receive buffers.
    kernel::klist<eth::rx_page> pages;
    for (size_t i=0; i<rx_qlen; ++i)
    {
        auto* p = new rx_page;
        pages.push_back(&p->link);
    }
    post_rx_pages(pages);

    // Post a dhcp request.
    issue_dhcp_discover(&dhcp_op);
    state = EI_STATE_WAIT_DHCP_DISCOVER_RESP_COMP;
}

void
eth::interface::handle_dhcp_send_comp()
{
    switch (state)
    {
        case EI_STATE_WAIT_DHCP_DISCOVER_RESP_COMP:
            // TODO: Arm a retry timer here?
            state = EI_STATE_WAIT_DHCP_DISCOVER_RESP;
        break;

        case EI_STATE_WAIT_DHCP_DISCOVER_COMP:
            handle_dhcp_discover_complete();
        break;

        case EI_STATE_WAIT_DHCP_REQUEST_RESP_COMP:
            // TODO: Arm a retry timer here?
            state = EI_STATE_WAIT_DHCP_REQUEST_RESP;
        break;

        case EI_STATE_WAIT_DHCP_REQUEST_COMP:
            handle_dhcp_request_complete();
        break;

        default:
            kernel::panic("dhcp send completion in wonky state!");
        break;
    }
}

void
eth::interface::handle_dhcp_offer_recv_comp(rx_page* p)
{
    auto* disc = (dhcp::eth_message*)(p->payload + p->eth_offset);

    auto* server_opt = disc->msg.find_option(54);
    if (!server_opt)
    {
        dump_rx_page(p);
        return;
    }

    switch (state)
    {
        case EI_STATE_WAIT_DHCP_DISCOVER_RESP_COMP:
            dump_good_page(p);
            dhcp_offer_addr = disc->msg.yiaddr;
            dhcp_server_id  = *(ipv4::addr*)server_opt->data;
            state           = EI_STATE_WAIT_DHCP_DISCOVER_COMP;
        break;

        case EI_STATE_WAIT_DHCP_DISCOVER_RESP:
            dump_good_page(p);
            dhcp_offer_addr = disc->msg.yiaddr;
            dhcp_server_id  = *(ipv4::addr*)server_opt->data;
            handle_dhcp_discover_complete();
        break;

        default:
            dump_rx_page(p);
        break;
    }
}

void
eth::interface::handle_dhcp_ack_recv_comp(rx_page* p)
{
    auto* disc = (dhcp::eth_message*)(p->payload + p->eth_offset);

    auto* sn_option  = disc->msg.find_option(1);
    auto* gw_option  = disc->msg.find_option(3);
    auto* dns_option = disc->msg.find_option(6);

    ipv4::addr sn_ip{0,0,0,0};
    if (sn_option)
        sn_ip = *(ipv4::addr*)sn_option->data;

    ipv4::addr gw_ip{0,0,0,0};
    if (gw_option)
        gw_ip = *(ipv4::addr*)gw_option->data;

    ipv4::addr dns_ip{0,0,0,0};
    if (dns_option)
        dns_ip = *(ipv4::addr*)dns_option->data;

    switch (state)
    {
        case EI_STATE_WAIT_DHCP_REQUEST_RESP_COMP:
            dump_good_page(p);
            dhcp_ack_addr        = disc->msg.yiaddr;
            dhcp_ack_subnet_mask = sn_ip;
            dhcp_ack_gw_addr     = gw_ip;
            dhcp_ack_dns_addr    = dns_ip;
            state = EI_STATE_WAIT_DHCP_REQUEST_COMP;
        break;

        case EI_STATE_WAIT_DHCP_REQUEST_RESP:
            dump_good_page(p);
            dhcp_ack_addr = disc->msg.yiaddr;
            dhcp_ack_subnet_mask = sn_ip;
            dhcp_ack_gw_addr     = gw_ip;
            dhcp_ack_dns_addr    = dns_ip;
            handle_dhcp_request_complete();
        break;

        default:
            dump_rx_page(p);
        break;
    }
}

void
eth::interface::handle_dhcp_nak_recv_comp(rx_page* p)
{
    kernel::panic("nak received :(\n");
}

void
eth::interface::handle_dhcp_recv_comp(rx_page* p)
{
    auto* disc = (dhcp::eth_message*)(p->payload + p->eth_offset);

    auto* type_opt = disc->msg.find_option(53);
    if (disc->msg.xid != dhcp_xid || !type_opt || type_opt->len != 1)
    {
        dump_rx_page(p);
        return;
    }

    switch (type_opt->data[0])
    {
        case dhcp::DHCP_OFFER:
            handle_dhcp_offer_recv_comp(p);
        break;

        case dhcp::DHCP_ACK:
            handle_dhcp_ack_recv_comp(p);
        break;

        case dhcp::DHCP_NAK:
            handle_dhcp_nak_recv_comp(p);
        break;

        default:
            dump_rx_page(p);
        break;
    }
}

void
eth::interface::handle_dhcp_discover_complete()
{
    switch (state)
    {
        case EI_STATE_WAIT_DHCP_DISCOVER_RESP:
        case EI_STATE_WAIT_DHCP_DISCOVER_COMP:
            kassert(dhcp_offer_addr.v != 0);
            issue_dhcp_request(&dhcp_op);
            state = EI_STATE_WAIT_DHCP_REQUEST_RESP_COMP;
        break;

        default:
            kernel::panic("dhcp completion in wonky state!");
        break;
    }
}

void
eth::interface::handle_dhcp_request_complete()
{
    switch (state)
    {
        case EI_STATE_WAIT_DHCP_REQUEST_RESP:
        case EI_STATE_WAIT_DHCP_REQUEST_COMP:
            printf("%02X:%02X:%02X:%02X:%02X:%02X: DHCP negotiated ["
                   "IP %u.%u.%u.%u  SN %u.%u.%u.%u  GW %u.%u.%u.%u  "
                   "DNS %u.%u.%u.%u]\n",
                   hw_mac[0],hw_mac[1],hw_mac[2],hw_mac[3],hw_mac[4],hw_mac[5],
                   dhcp_ack_addr[0],dhcp_ack_addr[1],
                   dhcp_ack_addr[2],dhcp_ack_addr[3],
                   dhcp_ack_subnet_mask[0],dhcp_ack_subnet_mask[1],
                   dhcp_ack_subnet_mask[2],dhcp_ack_subnet_mask[3],
                   dhcp_ack_gw_addr[0],dhcp_ack_gw_addr[1],
                   dhcp_ack_gw_addr[2],dhcp_ack_gw_addr[3],
                   dhcp_ack_dns_addr[0],dhcp_ack_dns_addr[1],
                   dhcp_ack_dns_addr[2],dhcp_ack_dns_addr[3]);
            state = EI_STATE_READY;
        break;

        default:
            kernel::panic("dhcp completion in wonky state!");
        break;
    }
}

void
eth::interface::handle_tx_completion(eth::tx_op* op)
{
    op->cb(op);
}

void
eth::interface::handle_rx_pages(kernel::klist<rx_page>& pages)
{
    while (!pages.empty())
    {
        auto* p = klist_front(pages,link);
        pages.pop_front();

        auto* h = (eth::header*)(p->payload + p->eth_offset);
        if ((h->dst_mac == hw_mac || h->dst_mac == eth::broadcast_addr) &&
            h->ethertype == 0x0800)
        {
            handle_rx_ipv4_frame(p);
        }
        else
        {
            free_pages.push_back(&p->link);
            dump_rx_page(p);
        }
    }
    post_rx_pages(free_pages);
}

void
eth::interface::handle_rx_ipv4_frame(rx_page* p)
{
    auto* h   = (eth::header*)(p->payload + p->eth_offset);
    auto* iph = (ipv4::header*)(h+1);
    if (iph->dst_ip != ip_addr && iph->dst_ip != ipv4::broadcast_addr)
    {
        dump_rx_page(p);
        free_pages.push_back(&p->link);
        return;
    }

    switch (iph->proto)
    {
        case 0x11:
            handle_rx_ipv4_udp_frame(p);
        break;

        default:
            dump_rx_page(p);
            free_pages.push_back(&p->link);
        break;
    }
}

void
eth::interface::handle_rx_ipv4_udp_frame(rx_page* p)
{
    auto* h   = (eth::header*)(p->payload + p->eth_offset);
    auto* iph = (ipv4::header*)(h+1);
    auto* uh  = (udp::header*)(iph+1);
    if (uh->src_port == 67 && uh->dst_port == 68)
    {
        handle_dhcp_recv_comp(p);
        free_pages.push_back(&p->link);
    }
    else
    {
        dump_rx_page(p);
        free_pages.push_back(&p->link);
    }
}

void
eth::interface::issue_dhcp_discover(tx_op* op)
{
    auto* disc = (dhcp::eth_message*)dhcp_tx_page.addr;
    disc->llhdr.dst_mac           = eth::broadcast_addr;
    disc->llhdr.src_mac           = hw_mac;
    disc->llhdr.ethertype         = 0x0800;
    disc->iphdr.version_ihl       = 0x45;
    disc->iphdr.dscp_ecn          = 0;
    disc->iphdr.total_len         = sizeof(*disc) - sizeof(disc->llhdr);
    disc->iphdr.identification    = 0;
    disc->iphdr.flags_fragoffset  = 0x4000;
    disc->iphdr.ttl               = 1;
    disc->iphdr.proto             = 0x11; // UDP
    disc->iphdr.src_ip            = ipv4::addr{0,0,0,0};
    disc->iphdr.dst_ip            = ipv4::broadcast_addr;
    disc->iphdr.header_checksum   = ipv4::csum(&disc->iphdr);
    disc->uhdr.src_port           = 68;
    disc->uhdr.dst_port           = 67;
    disc->uhdr.len                = sizeof(*disc) - sizeof(disc->llhdr) -
                                    sizeof(disc->iphdr);
    disc->uhdr.checksum           = 0; // Disable checksumming
    disc->msg.format_discover(++dhcp_xid,hw_mac);

    op->cb            = handle_dhcp_send_comp_bounce;
    op->nalps         = 1;
    op->alps[0].paddr = (kernel::dma_addr64)disc;
    op->alps[0].len   = sizeof(*disc);
    post_tx_frame(op);
}

void
eth::interface::issue_dhcp_request(tx_op* op)
{
    auto* disc = (dhcp::eth_message*)dhcp_tx_page.addr;
    disc->llhdr.dst_mac           = eth::broadcast_addr;
    disc->llhdr.src_mac           = hw_mac;
    disc->llhdr.ethertype         = 0x0800;
    disc->iphdr.version_ihl       = 0x45;
    disc->iphdr.dscp_ecn          = 0;
    disc->iphdr.total_len         = sizeof(*disc) - sizeof(disc->llhdr);
    disc->iphdr.identification    = 0;
    disc->iphdr.flags_fragoffset  = 0x4000;
    disc->iphdr.ttl               = 1;
    disc->iphdr.proto             = 0x11; // UDP
    disc->iphdr.src_ip            = ipv4::addr{0,0,0,0};
    disc->iphdr.dst_ip            = ipv4::broadcast_addr;
    disc->iphdr.header_checksum   = ipv4::csum(&disc->iphdr);
    disc->uhdr.src_port           = 68;
    disc->uhdr.dst_port           = 67;
    disc->uhdr.len                = sizeof(*disc) - sizeof(disc->llhdr) -
                                    sizeof(disc->iphdr);
    disc->uhdr.checksum           = 0; // Disable checksumming
    disc->msg.format_request(dhcp_xid,hw_mac,dhcp_offer_addr,dhcp_server_id);

    op->cb            = handle_dhcp_send_comp_bounce;
    op->nalps         = 1;
    op->alps[0].paddr = (kernel::dma_addr64)disc;
    op->alps[0].len   = sizeof(*disc);
    post_tx_frame(op);
}
