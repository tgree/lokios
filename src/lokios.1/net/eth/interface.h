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

        // Size of the hardware transmit and receive queues.
        const size_t        tx_qlen;
        const size_t        rx_qlen;
        size_t              rx_posted_count;

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
                void    activate();
                void    refill_rx_pages();

        // Transmit a frame.
        virtual void    post_tx_frame(net::tx_op* op) = 0;

        // Post a receive page.  The entire page is available for the driver's
        // use even if the chip doesn't support a large enough MTU to use it
        // efficiently.
        virtual void    post_rx_pages(kernel::klist<net::rx_page>& pages) = 0;

        // Handle link status changes.
                void    handle_link_up(size_t mbits, bool full_duplex);
                void    handle_link_down();

        // Handle DHCP status updates.
                void    handle_dhcp_success();
                void    handle_dhcp_failure();

        // Handle send and receive completions.
                void    handle_tx_completion(net::tx_op* op);
                void    handle_rx_pages(kernel::klist<net::rx_page>& pages);
                void    handle_rx_ipv4_frame(net::rx_page* p);
                void    handle_rx_ipv4_tcp_frame(net::rx_page* p);
                void    handle_rx_ipv4_udp_frame(net::rx_page* p);
                void    handle_rx_arp_frame(net::rx_page* p);

        interface(const eth::addr& hw_mac, size_t tx_qlen, size_t rx_qlen);
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_ETH_INTERFACE_H */
