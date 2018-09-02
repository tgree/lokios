#ifndef __KERNEL_NET_ETH_INTERFACE_H
#define __KERNEL_NET_ETH_INTERFACE_H

#include "addr.h"
#include "net/interface.h"
#include "net/net.h"
#include "net/ip/ip.h"

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
        dhcp::client*       dhcpc;

        // ARP service.
        arp::service<eth::net_traits,ipv4::net_traits>* arpc_ipv4;

        // For sending frames.
        virtual size_t  format_ll_reply(net::rx_page* p, void* ll_hdr,
                                        size_t ll_hdr_len);
        virtual size_t  format_arp_broadcast(void* arp_payload);

        // Access the PHY.  These are asynchronous and result in callbacks.
                void    issue_probe_phy(kernel::wqe* cqe);
        virtual void    issue_phy_read_16(uint8_t offset, kernel::wqe* cqe) = 0;
        virtual void    issue_phy_write_16(uint16_t v, uint8_t offset,
                                           kernel::wqe* cqe) = 0;

        // Handle DHCP status updates.
                void    handle_dhcp_success();
                void    handle_dhcp_failure();

        // Handle receive completions.
                void    handle_rx_pages(kernel::klist<net::rx_page>& pages);
                void    handle_rx_arp_frame(net::rx_page* p);

        // Helpers.
        virtual void    dump_arp_table();

        interface(const eth::addr& hw_mac, size_t tx_qlen, size_t rx_qlen,
                  uint16_t tx_mtu, uint16_t rx_mtu);
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_ETH_INTERFACE_H */
