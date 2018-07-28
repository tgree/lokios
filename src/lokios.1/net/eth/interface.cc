#include "interface.h"
#include "traits.h"
#include "phy/phy.h"
#include "net/arp/arp.h"
#include "net/dhcp/dhcp.h"
#include "net/dhcp/dhcpc.h"
#include "net/tcp/traits.h"
#include "kernel/console.h"
#include "kernel/mm/vm.h"
#include "acpi/tables.h"
#include "platform/platform.h"

#define DUMP_GOOD_PACKETS 0
#if DUMP_GOOD_PACKETS
#define dump_good_page(p) dump_rx_page(p)
#else
#define dump_good_page(p)
#endif

using kernel::console::printf;
using kernel::_kassert;

static kernel::spinlock slab_lock;
static kernel::slab tcp_slab(sizeof(tcp::listener));

eth::interface::interface(const eth::addr& hw_mac, size_t tx_qlen,
    size_t rx_qlen):
        net::interface(tx_qlen,rx_qlen),
        hw_mac(hw_mac),
        phy(NULL)
{
    intf_dbg("creating interface with MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
             hw_mac[0],hw_mac[1],hw_mac[2],hw_mac[3],hw_mac[4],hw_mac[5]);

    dhcpc = new dhcp::client(this);
    arpc_ipv4 = new arp::service<eth::net_traits,ipv4::net_traits>(this);
}

eth::interface::~interface()
{
}

void
eth::interface::issue_probe_phy(kernel::work_entry* cqe)
{
    eth::phy_driver::issue_probe(this,cqe);
}

void
eth::interface::tcp_listen(uint16_t port, tcp::connection_filter f)
{
    kassert(!intf_mem->tcp_listeners[port]);

    tcp::listener* l;
    with (slab_lock)
    {
        l = tcp_slab.alloc<tcp::listener>();
    }

    l->intf                       = this;
    l->port                       = port;
    l->filter_delegate            = f;
    intf_mem->tcp_listeners[port] = l;
}

void
eth::interface::activate()
{
    // Activate it in the network stack.
    net::interface::activate();

    // Post a dhcp request.
    // TODO: Wait a random time from 0-10 seconds before starting dhcpc.
    dhcpc->start();
}

void
eth::interface::handle_link_up(size_t mbit, bool full_duplex)
{
    intf_dbg("link up at %lu Mbit %s duplex\n",
             mbit,full_duplex ? "full" : "half");
}

void
eth::interface::handle_link_down()
{
    intf_dbg("link down\n");
}

void
eth::interface::handle_dhcp_success()
{
    intf_dbg("DHCP negotiated [IP %u.%u.%u.%u  SN %u.%u.%u.%u  "
           "GW %u.%u.%u.%u  DNS %u.%u.%u.%u  Lease %u  T1 %u  T2 %u]\n",
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

    // If there is an 'iTST' ACPI table, this indicates we are running on qemu
    // in integration-tes mode and should just exit instead of spinning in the
    // scheduler forever.
    if (kernel::find_acpi_table('TSTi'))
    {
        printf("Kernel exiting successfully.\n");
        kernel::exit_guest(1);
    }
}

void
eth::interface::handle_dhcp_failure()
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X: DHCP failed\n",
           hw_mac[0],hw_mac[1],hw_mac[2],hw_mac[3],hw_mac[4],hw_mac[5]);
}

void
eth::interface::handle_rx_pages(kernel::klist<net::rx_page>& pages)
{
    while (!pages.empty())
    {
        auto* p  = klist_front(pages,link);
        p->flags = 0;
        pages.pop_front();
        --rx_posted_count;

        auto* h = (eth::header*)(p->payload + p->pay_offset);
        if (h->dst_mac == hw_mac || h->dst_mac == eth::broadcast_addr)
        {
            // Strip the eth::header.
            p->pay_offset += sizeof(eth::header);
            p->pay_len    -= sizeof(eth::header);
            switch (h->ether_type)
            {
                case 0x0800:    handle_rx_ipv4_frame(p);    break;
                case 0x0806:    handle_rx_arp_frame(p);     break;
            }
        }
        if (!(p->flags & NRX_FLAG_NO_DELETE))
            delete p;
    }
    refill_rx_pages();
}

void
eth::interface::handle_rx_arp_frame(net::rx_page* p)
{
    auto* sp = (arp::header*)(p->payload + p->pay_offset);
    switch (sp->ptype)
    {
        case 0x0800:    arpc_ipv4->handle_rx_frame(p);  break;
    }
}
