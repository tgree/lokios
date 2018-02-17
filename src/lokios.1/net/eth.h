#ifndef __KERNEL_NET_ETH_H
#define __KERNEL_NET_ETH_H

#include "ip.h"
#include "hdr/compiler.h"
#include "kernel/types.h"
#include "kernel/kassert.h"
#include "mm/mm.h"
#include <stdint.h>
#include <stddef.h>

namespace eth
{
    // MAC address.
    struct addr
    {
        uint8_t v[6];

        void operator=(uint64_t _v)
        {
            v[0] = (_v >> 40);
            v[1] = (_v >> 32);
            v[2] = (_v >> 24);
            v[3] = (_v >> 16);
            v[4] = (_v >>  8);
            v[5] = (_v >>  0);
        }
        uint8_t operator[](size_t i) const {return v[i];}
    };
    constexpr bool operator==(const addr& lhs, const addr& rhs)
    {
        return lhs.v[0] == rhs.v[0] &&
               lhs.v[1] == rhs.v[1] &&
               lhs.v[2] == rhs.v[2] &&
               lhs.v[3] == rhs.v[3] &&
               lhs.v[4] == rhs.v[4] &&
               lhs.v[5] == rhs.v[5];
    }
    constexpr bool operator!=(const addr& lhs, const addr& rhs)
    {
        return !(lhs == rhs);
    }

    constexpr const addr broadcast_addr{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    // Ethernet frame header.
    struct header
    {
        typedef eth::addr addr_type;

        eth::addr   dst_mac;
        eth::addr   src_mac;
        be_uint16_t ethertype;
    } __PACKED__;
    KASSERT(sizeof(header) == 14);

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
    struct rx_page
    {
        kernel::klink   link;
        uint16_t        eth_offset;
        uint16_t        eth_len;
        uint8_t         payload[4084];
    };
    KASSERT(sizeof(rx_page) == PAGE_SIZE);

    // Ethernet interface.
    struct interface
    {
        // State.
        enum
        {
            EI_STATE_WAIT_ACTIVATE,

            EI_STATE_WAIT_DHCP_DISCOVER_RESP_COMP,
            EI_STATE_WAIT_DHCP_DISCOVER_RESP,
            EI_STATE_WAIT_DHCP_DISCOVER_COMP,

            EI_STATE_WAIT_DHCP_REQUEST_RESP_COMP,
            EI_STATE_WAIT_DHCP_REQUEST_RESP,
            EI_STATE_WAIT_DHCP_REQUEST_COMP,

            EI_STATE_READY,
        } state;

        // MAC address that was assigned by hardware.
        const eth::addr     hw_mac;

        // IP address assigned by software.
        ipv4::addr          ip_addr;

        // Size of the hardware transmit and receive queues.
        const size_t        tx_qlen;
        const size_t        rx_qlen;

        // Timer for the state machine.
        kernel::work_entry      timer_wqe;
        kernel::page_raii       dhcp_tx_page;
        ipv4::addr              dhcp_offer_addr;
        ipv4::addr              dhcp_server_id;
        ipv4::addr              dhcp_ack_addr;
        ipv4::addr              dhcp_ack_subnet_mask;
        ipv4::addr              dhcp_ack_gw_addr;
        ipv4::addr              dhcp_ack_dns_addr;
        eth::tx_op              dhcp_op;
        uint32_t                dhcp_xid;
        kernel::klist<rx_page>  free_pages;

        // Activate the interface.
                void    activate();
                void    handle_dhcp_send_comp();
                void    handle_dhcp_recv_comp(rx_page* p);
                void    handle_dhcp_offer_recv_comp(rx_page* p);
                void    handle_dhcp_ack_recv_comp(rx_page* p);
                void    handle_dhcp_nak_recv_comp(rx_page* p);
                void    handle_dhcp_discover_complete();
                void    handle_dhcp_request_complete();

        // Transmit a frame.
        virtual void    post_tx_frame(tx_op* op) = 0;

        // Post a receive page.  The entire page is available for the driver's
        // use even if the chip doesn't support a large enough MTU to use it
        // efficiently.
        virtual void    post_rx_pages(kernel::klist<rx_page>& pages) = 0;

        // Handle send and receive completions.
                void    handle_tx_completion(eth::tx_op* op);
                void    handle_rx_pages(kernel::klist<rx_page>& pages);
                void    handle_rx_ipv4_frame(rx_page* p);
                void    handle_rx_ipv4_udp_frame(rx_page* p);

        // Issue different types of packet.
                void    issue_dhcp_discover(tx_op* op);
                void    issue_dhcp_request(tx_op* op);

        interface(const eth::addr& hw_mac, size_t tx_qlen, size_t rx_qlen);
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_ETH_H */
