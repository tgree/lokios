#ifndef __KERNEL_NET_ETH_INTERFACE_H
#define __KERNEL_NET_ETH_INTERFACE_H

#include "addr.h"
#include "net/ip/ip.h"
#include "kernel/schedule.h"
#include "kernel/types.h"
#include "kernel/kassert.h"
#include "mm/mm.h"
#include "mm/page.h"

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

    // Transmit parameter block.
    struct tx_op
    {
        kernel::klink   link;
        void           (*cb)(tx_op* op);
        uint64_t        rsrv;
        size_t          nalps;
        kernel::dma_alp alps[2];
    };
    KASSERT(sizeof(tx_op) == 64);

    // Receive parameter block.  The eth_offset and eth_len fields are the
    // offsets from the start of the payload to the beginning of the Ethernet
    // frame (some drivers may require driver-specific headers preceding the
    // Ethernet part) and the length of the Ethernet frame (not including any
    // driver-specific headers).
    //
    // Note: the eth_len field is the length of the entire Ethernet frame.  The
    // minimum payload size for the Ethernet frame's payload field is 46 (or 42,
    // if 802.1Q tagging is enabled), so the payload field will be padded on
    // the send side for short packets.  In other words, the eth_len field is
    // an upper bound on the usable payload, but not a tight upper bound.
    struct rx_page
    {
        kernel::klink   link;
        uint16_t        eth_offset;
        uint16_t        eth_len;
        uint8_t         payload[4084];
    };
    KASSERT(sizeof(rx_page) == PAGE_SIZE);

    // Handler for inbound frames.
    struct interface;
    typedef void (*frame_handler)(interface* intf, void* cookie, rx_page* p);

    // Method delegate for inbound frames.
    template<typename T, void (T::*Handler)(interface* intf, rx_page*)>
    struct _frame_delegate
    {
        static void handler(interface* intf, void* cookie, rx_page* p)
        {
            (((T*)cookie)->*Handler)(intf,p);
        }
    };
#define frame_delegate(fn) \
    eth::_frame_delegate< \
        std::remove_reference_t<decltype(*this)>, \
        &std::remove_reference_t<decltype(*this)>::fn>::handler

    // Data structure that gets mapped at the interface's reserved vaddr.
    struct interface_mem
    {
        struct
        {
            void*           cookie;
            frame_handler   handler;
        } udp_frame_handlers[65536];
    };

    // Ethernet interface.
    struct interface
    {
        // The ethX id number.
        const size_t        id;

        // Interface memory.
        interface_mem*      intf_mem;

        // MAC address that was assigned by hardware.
        const eth::addr     hw_mac;

        // IP address assigned by software.
        ipv4::addr          ip_addr;

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

        // Register UDP frame handlers.
        inline  void    register_udp_handler(uint16_t port, void* cookie,
                                             frame_handler handler)
        {
            auto* ufh = &intf_mem->udp_frame_handlers[port];
            kernel::kassert(!ufh->handler);
            ufh->cookie  = cookie;
            ufh->handler = handler;
        }
        inline  void    deregister_udp_handler(uint16_t port)
        {
            auto* ufh = &intf_mem->udp_frame_handlers[port];
            kernel::kassert(ufh->handler);
            ufh->handler = NULL;
        }

        // Activate the interface.
                void    activate();
                void    refill_rx_pages();

        // Transmit a frame.
        virtual void    post_tx_frame(tx_op* op) = 0;

        // Post a receive page.  The entire page is available for the driver's
        // use even if the chip doesn't support a large enough MTU to use it
        // efficiently.
        virtual void    post_rx_pages(kernel::klist<rx_page>& pages) = 0;

        // Handle link status changes.
                void    handle_link_up(size_t mbits, bool full_duplex);
                void    handle_link_down();

        // Handle DHCP status updates.
                void    handle_dhcp_success();
                void    handle_dhcp_failure();

        // Handle send and receive completions.
                void    handle_tx_completion(eth::tx_op* op);
                void    handle_rx_pages(kernel::klist<rx_page>& pages);
                void    handle_rx_ipv4_frame(rx_page* p);
                void    handle_rx_ipv4_udp_frame(rx_page* p);
                void    handle_rx_arp_frame(rx_page* p);

        interface(const eth::addr& hw_mac, size_t tx_qlen, size_t rx_qlen);
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_ETH_INTERFACE_H */
