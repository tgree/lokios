#include "interface.h"
#include "traits.h"
#include "phy/phy.h"
#include "net/arp/arp.h"
#include "net/dhcp/dhcp.h"
#include "net/dhcp/dhcpc.h"
#include "net/tcp/traits.h"
#include "kern/console.h"
#include "mm/vm.h"
#include "platform/platform.h"

#define DUMP_GOOD_PACKETS 0
#if DUMP_GOOD_PACKETS
#define dump_good_page(p) dump_rx_page(p)
#else
#define dump_good_page(p)
#endif

using kernel::console::printf;
using kernel::_kassert;

eth::interface::interface(const eth::addr& hw_mac, size_t tx_qlen,
    size_t rx_qlen, uint16_t tx_mtu, uint16_t rx_mtu):
        net::interface(tx_qlen,rx_qlen,tx_mtu,rx_mtu),
        hw_mac(hw_mac),
        phy(NULL),
        arp_node(&wapi_node,method_delegate(handle_arp_wapi_request),
                 METHOD_GET_MASK,"arp")
{
    intf_dbg("creating interface with MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
             hw_mac[0],hw_mac[1],hw_mac[2],hw_mac[3],hw_mac[4],hw_mac[5]);

    dhcpc = new dhcp::client(this);
    arpc_ipv4 = new arp::service<eth::net_traits,ipv4::net_traits>(this);
}

eth::interface::~interface()
{
}

size_t
eth::interface::format_ll_reply(net::rx_page* p, void* ll_hdr,
    size_t ll_hdr_len)
{
    kassert(ll_hdr_len >= sizeof(eth::header));

    // Unstrip the rx_page to find the request header.
    auto* h = p->llhdr_cast<eth::header*>();
    kassert(h->dst_mac == hw_mac ||
            h->dst_mac == eth::net_traits::broadcast_addr);
    
    // Format the reply header.
    auto* rh       = (eth::header*)ll_hdr;
    rh->src_mac    = hw_mac;
    rh->dst_mac    = h->src_mac;
    rh->ether_type = h->ether_type;

    return sizeof(eth::header);
}

size_t
eth::interface::format_arp_broadcast(void* arp_payload)
{
    // Format the broadcast header.
    auto* h       = (eth::header*)((char*)arp_payload - sizeof(eth::header));
    h->src_mac    = hw_mac;
    h->dst_mac    = eth::net_traits::broadcast_addr;
    h->ether_type = 0x0806;

    return sizeof(eth::header);
}

void
eth::interface::issue_probe_phy(kernel::wqe* cqe)
{
    eth::phy_driver::issue_probe(this,cqe);
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
        auto* p        = klist_front(pages,link);
        uint64_t flags = 0;
        pages.pop_front();
        --rx_posted_count;

        if (p->pay_len >= sizeof(eth::header))
        {
            auto* h = (eth::header*)(p->payload + p->pay_offset);
            if (h->dst_mac == hw_mac || h->dst_mac == eth::broadcast_addr)
            {
                // Strip the eth::header.
                p->pay_offset += sizeof(eth::header);
                p->pay_len    -= sizeof(eth::header);
                switch (h->ether_type)
                {
                    case 0x0806:         handle_rx_arp_frame(p);    break;
                    case 0x0800: flags = handle_rx_ipv4_frame(p);   break;
                }
            }
        }
        if (!(flags & NRX_FLAG_NO_DELETE))
            free_rx_page(p);
    }
    refill_rx_pages();
}

void
eth::interface::handle_rx_arp_frame(net::rx_page* p)
{
    auto* sp = p->payload_cast<arp::header*>();
    switch (sp->ptype)
    {
        case 0x0800:    arpc_ipv4->handle_rx_frame(p);  break;
    }
}

void
eth::interface::dump_arp_table()
{
    size_t i = 0;
    intf_dbg("arp table:\n");
    auto* ht = &arpc_ipv4->arp_entries;
    for (size_t j=0; j<ht->nbins; ++j)
    {
        for (auto& e : klist_elems(ht->bins[j],link))
        {
            ++i;
            intf_dbg("%2zu: %u.%u.%u.%u: %02X:%02X:%02X:%02X:%02X:%02X\n",
                     i,e.k[0],e.k[1],e.k[2],e.k[3],
                     e.v[0],e.v[1],e.v[2],e.v[3],e.v[4],e.v[5]);
        }
    }
}

void
eth::interface::handle_arp_wapi_request(wapi::node* node, http::request* req,
    json::object* obj, http::response* rsp)
{
    rsp->printf("{\r\n"
                "    \"entries\" : { ");
    for (auto& n : arpc_ipv4->arp_entries)
    {
        rsp->printf("\r\n        "
                    "\"%u.%u.%u.%u\" : \"%02X:%02X:%02X:%02X:%02X:%02X\",",
                    n.k[0],n.k[1],n.k[2],n.k[3],
                    n.v[0],n.v[1],n.v[2],n.v[3],n.v[4],n.v[5]);
    }
    rsp->ks.shrink();
    rsp->printf("\r\n        }\r\n}\r\n");
}

void
eth::interface::handle_wapi_request(wapi::node* node, http::request* req,
    json::object* obj, http::response* rsp)
{
    rsp->printf("{\r\n"
                "    \"type\"        : \"ethernet\",\r\n"
                "    \"mac\"         : \"%02X:%02X:%02X:%02X:%02X:%02X\",\r\n"
                "    \"ip\"          : \"%u.%u.%u.%u\",\r\n"
                "    \"subnet_mask\" : \"%u.%u.%u.%u\",\r\n"
                "    \"gateway_ip\"  : \"%u.%u.%u.%u\",\r\n"
                "    \"dns_ip\"      : \"%u.%u.%u.%u\",\r\n"
                "    \"tx_mtu\"      : %u,\r\n"
                "    \"rx_mtu\"      : %u,\r\n"
                "    \"tq_len\"      : %zu,\r\n"
                "    \"rq_len\"      : %zu,",
                hw_mac[0],hw_mac[1],hw_mac[2],hw_mac[3],hw_mac[4],hw_mac[5],
                ip_addr[0],ip_addr[1],ip_addr[2],ip_addr[3],
                dhcpc->subnet_mask[0],dhcpc->subnet_mask[1],
                dhcpc->subnet_mask[2],dhcpc->subnet_mask[3],
                dhcpc->gw_addr[0],dhcpc->gw_addr[1],
                dhcpc->gw_addr[2],dhcpc->gw_addr[3],
                dhcpc->dns_addr[0],dhcpc->dns_addr[1],
                dhcpc->dns_addr[2],dhcpc->dns_addr[3],
                tx_mtu,rx_mtu,tx_qlen,rx_qlen);
    add_driver_wapi_info(rsp);
    rsp->ks.shrink();
    rsp->printf("\r\n}\r\n");
}
