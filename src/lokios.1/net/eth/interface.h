#ifndef __KERNEL_NET_ETH_INTERFACE_H
#define __KERNEL_NET_ETH_INTERFACE_H

#include "addr.h"
#include "net/interface.h"
#include "net/net.h"
#include "net/ip/ip.h"
#include "net/tcp/socket.h"
#include "kernel/schedule.h"
#include "kernel/types.h"
#include "kernel/kassert.h"
#include "mm/mm.h"
#include "mm/page.h"
#include <stdarg.h>

namespace dhcp
{
    struct client;
}

namespace arp
{
    template<typename hw_traits, typename proto_traits> struct service;
}

namespace eth
{
    struct phy;
    struct net_traits;

    // Ethernet interface.
    struct interface : public net::interface
    {
        // MAC address that was assigned by hardware.
        const eth::addr     hw_mac;

        // PHY, if present.
        eth::phy*           phy;

        // DHCP client service.
        dhcp::client*           dhcpc;

        // ARP service.
        arp::service<eth::net_traits,ipv4::net_traits>* arpc_ipv4;

        // Access the PHY.  These are asynchronous and result in callbacks.
                void        issue_probe_phy(kernel::work_entry* cqe);
        virtual void        issue_phy_read_16(uint8_t offset,
                                kernel::work_entry* cqe) = 0;
        virtual void        issue_phy_write_16(uint16_t v, uint8_t offset,
                                kernel::work_entry* cqe) = 0;

        // TCP.
                void    tcp_listen(uint16_t port, tcp::connection_filter f);
                int     tcp_filter(const tcp::header* syn);

        // Activate the interface.
        virtual void    activate();

        // Handle link status changes.
                void    handle_link_up(size_t mbits, bool full_duplex);
                void    handle_link_down();

        // Handle DHCP status updates.
                void    handle_dhcp_success();
                void    handle_dhcp_failure();

        // Handle receive completions.
                void    handle_rx_pages(kernel::klist<net::rx_page>& pages);
                void    handle_rx_arp_frame(net::rx_page* p);

        interface(const eth::addr& hw_mac, size_t tx_qlen, size_t rx_qlen);
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_ETH_INTERFACE_H */
