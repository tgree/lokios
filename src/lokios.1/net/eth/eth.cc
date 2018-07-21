#include "eth.h"
#include "phy/phy.h"
#include "net/dhcp.h"
#include "net/dhcpc.h"
#include "net/arp.h"
#include "kernel/console.h"
#include "kernel/mm/vm.h"

#define DUMP_GOOD_PACKETS 0
#if DUMP_GOOD_PACKETS
#define dump_good_page(p) dump_rx_page(p)
#else
#define dump_good_page(p)
#endif

#define intf_dbg(fmt,...) \
    kernel::console::printf("eth%zu: " fmt,id,##__VA_ARGS__)

using kernel::console::printf;
using kernel::_kassert;

static kernel::spinlock ids_lock;
static uint32_t free_ids = 0xFFFFFFFF;

static size_t
alloc_id()
{
    with (ids_lock)
    {
        kassert(free_ids != 0);
        size_t id = kernel::ffs(free_ids);
        free_ids &= ~(1<<id);
        return id;
    }
}

static void
free_id(size_t id)
{
    with (ids_lock)
    {
        free_ids |= (1<<id);
    }
}

eth::interface::interface(const eth::addr& hw_mac, size_t tx_qlen,
    size_t rx_qlen):
        id(alloc_id()),
        intf_mem((interface_mem*)(MM_ETH_BEGIN + id*MM_ETH_STRIDE)),
        hw_mac(hw_mac),
        ip_addr{0,0,0,0},
        tx_qlen(tx_qlen),
        rx_qlen(rx_qlen),
        rx_posted_count(0),
        phy(NULL)
{
    intf_dbg("creating interface with MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
             hw_mac[0],hw_mac[1],hw_mac[2],hw_mac[3],hw_mac[4],hw_mac[5]);

    kernel::vmmap(intf_mem,sizeof(*intf_mem));
    memset(intf_mem,0,sizeof(*intf_mem));

    dhcpc = new dhcp::client(this);
    arpc_ipv4 = new arp::service<eth::net_traits,ipv4::net_traits>(this);
}

eth::interface::~interface()
{
    kernel::vmunmap(intf_mem,sizeof(*intf_mem));
    free_id(id);
}

void
eth::interface::issue_probe_phy(kernel::work_entry* cqe)
{
    eth::phy_driver::issue_probe(this,cqe);
}

void
eth::interface::activate()
{
    // Post receive buffers.
    refill_rx_pages();

    // Post a dhcp request.
    // TODO: Wait a random time from 0-10 seconds before starting dhcpc.
    dhcpc->start();
}

void
eth::interface::refill_rx_pages()
{
    if (rx_posted_count == rx_qlen)
        return;

    kernel::klist<eth::rx_page> pages;
    size_t n = rx_qlen - rx_posted_count;
    for (size_t i=0; i<n; ++i)
    {
        auto* p = new rx_page;
        pages.push_back(&p->link);
    }
    rx_posted_count = rx_qlen;
    post_rx_pages(pages);
}

void
eth::interface::handle_dhcp_success()
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X: DHCP negotiated ["
           "IP %u.%u.%u.%u  SN %u.%u.%u.%u  GW %u.%u.%u.%u  "
           "DNS %u.%u.%u.%u  Lease %u  T1 %u  T2 %u]\n",
           hw_mac[0],hw_mac[1],hw_mac[2],hw_mac[3],hw_mac[4],hw_mac[5],
           dhcpc->requested_addr[0],dhcpc->requested_addr[1],
           dhcpc->requested_addr[2],dhcpc->requested_addr[3],
           dhcpc->subnet_mask[0],dhcpc->subnet_mask[1],
           dhcpc->subnet_mask[2],dhcpc->subnet_mask[3],
           dhcpc->gw_addr[0],dhcpc->gw_addr[1],
           dhcpc->gw_addr[2],dhcpc->gw_addr[3],
           dhcpc->dns_addr[0],dhcpc->dns_addr[1],
           dhcpc->dns_addr[2],dhcpc->dns_addr[3],
           dhcpc->lease,dhcpc->t1,dhcpc->t2);
    ip_addr = dhcpc->requested_addr;
}

void
eth::interface::handle_dhcp_failure()
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X: DHCP failed\n",
           hw_mac[0],hw_mac[1],hw_mac[2],hw_mac[3],hw_mac[4],hw_mac[5]);
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
        --rx_posted_count;

        auto* h = (eth::header*)(p->payload + p->eth_offset);
        if (h->dst_mac != hw_mac && h->dst_mac != eth::broadcast_addr)
            delete p;
        else switch (h->ether_type)
        {
            case 0x0800:    handle_rx_ipv4_frame(p);    break;
            case 0x0806:    handle_rx_arp_frame(p);     break;
            default:        delete p;                   break;
        }
    }
    refill_rx_pages();
}

void
eth::interface::handle_rx_ipv4_frame(rx_page* p)
{
    auto* h   = (eth::header*)(p->payload + p->eth_offset);
    auto* iph = (ipv4::header*)(h+1);
    if (iph->dst_ip != ip_addr && iph->dst_ip != ipv4::broadcast_addr)
        delete p;
    else switch (iph->proto)
    {
        case 0x11:  handle_rx_ipv4_udp_frame(p);    break;
        default:    delete p;                       break;
    }
}

void
eth::interface::handle_rx_ipv4_udp_frame(rx_page* p)
{
    auto* h   = (eth::header*)(p->payload + p->eth_offset);
    auto* iph = (ipv4::header*)(h+1);
    auto* uh  = (udp::header*)(iph+1);
    auto* ufh = &intf_mem->udp_frame_handlers[uh->dst_port];
    if (ufh->handler)
        (*ufh->handler)(this,ufh->cookie,p);
    delete p;
}

void
eth::interface::handle_rx_arp_frame(rx_page* p)
{
    auto* h  = (eth::header*)(p->payload + p->eth_offset);
    auto* sp = (arp::short_payload*)(h+1);
    switch (sp->ptype)
    {
        case 0x0800:    arpc_ipv4->handle_rx_frame(p);  break;
    }
    delete p;
}
