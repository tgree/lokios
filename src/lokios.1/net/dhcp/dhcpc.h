#ifndef __KERNEL_NET_DHCPC_H
#define __KERNEL_NET_DHCPC_H

#include "dhcp.h"
#include "net/eth/interface.h"

#define ARP_RETRY_ATTEMPTS  5
#define ARP_TIMEOUT_MS      100

namespace dhcp
{
    struct client
    {
        // Current state.
        enum client_state_type
        {
            // Idle until we are told to send a DHCPDISCOVER.
            DHCP_INIT,

            // DHCPDISCOVER sent.  We collect DHCPOFFER responses and wait for
            // the DHCPDISCOVER send completion.  We then send a DHCPREQUEST.
            DHCP_SELECTING_WAIT_RX_RESP_TX_COMP,
            DHCP_SELECTING_WAIT_RX_RESP,
            DHCP_SELECTING_WAIT_TX_COMP,

            // DHCPREQUEST sent.  We wait for a DHCPACK or DHCPNAK response.  A
            // DHCPACK response doesn't necessarily mean we get the address.
            DHCP_REQUESTING_WAIT_RX_RESP_TX_COMP,
            DHCP_REQUESTING_WAIT_RX_RESP,
            DHCP_REQUESTING_WAIT_TX_COMP,

            // ARP lookup sent.  We wait for the ARP reply (in the case of a
            // duplicate address already on the network) or a timeout.
            DHCP_REQUESTING_WAIT_ARP_COMP,

            // DHCPDECLINE sent.  We received a DHCPACK but when we went to
            // verify via ARP that the address was available we detected that
            // it was already in use by someone else on the network.
            DHCP_DECLINED_WAIT_TX_COMP,

            // Bound state.  On entry we set T1 and T2 and start the lease.
            DHCP_BOUND_WAIT_TIMEOUT,

            // Unicast DHCPREQUEST sent.  T1 expired and it is time to start
            // renewing the lease with our current server.
            DHCP_RENEWING_WAIT_RX_RESP_TX_COMP,
            DHCP_RENEWING_WAIT_RX_RESP,
            DHCP_RENEWING_WAIT_TX_COMP,
            DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_T2_EXPIRED,
            DHCP_RENEWING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED,
            DHCP_RENEWING_WAIT_TX_COMP_T2_EXPIRED,
            DHCP_RENEWING_WAIT_TX_COMP_LEASE_EXPIRED,

            // Broadcast DHCPREQUEST sent.  T2 expired and we never got a
            // response from our original server.
            DHCP_REBINDING_WAIT_RX_RESP_TX_COMP,
            DHCP_REBINDING_WAIT_RX_RESP,
            DHCP_REBINDING_WAIT_TX_COMP,
            DHCP_REBINDING_WAIT_RX_RESP_TX_COMP_LEASE_EXPIRED,
            DHCP_REBINDING_WAIT_TX_COMP_LEASE_EXPIRED,
        } state;

        // The interface we are negotiating on behalf of.
        eth::interface*     intf;

        // Send op for any packet we are transmitting.
        net::tx_op          send_op;

        // State machine timers: T1, T2 and lease timer.
        kernel::tqe         t1_wqe;
        kernel::tqe         t2_wqe;
        kernel::tqe         lease_timer_wqe;

        // The transaction ID we are currently using.
        uint32_t            xid;

        // The address we are requesting.
        ipv4::addr          requested_addr;

        // The server we are negotiating with.
        eth::addr           server_mac;
        ipv4::addr          server_ip;

        // Parameters we learnt along the way.
        ipv4::addr          subnet_mask;
        ipv4::addr          gw_addr;
        ipv4::addr          dns_addr;
        uint32_t            lease;
        uint32_t            t2;
        uint32_t            t1;

        // ARP stuff.
        kernel::wqe         arp_cqe;
        eth::addr           arp_eth_addr;
        size_t              arp_attempt;

        // The dhcp message we will transmit.
        kernel::tqe         rx_dropped_timer;
        dhcp::eth_message   packet;

        // Transition helpers.
        void    TRANSITION_WAIT_RX_RESP();
        void    TRANSITION_WAIT_TX_COMP();

        // Post the currently-formatted packet.
        void    post_packet();

        // Start negotiation.
        void    start();
        void    start_selecting();
        void    start_declining();
        void    start_requesting();
        void    start_renewing();
        void    start_rebinding();

        // Process the information we were gathering in a given state.
        void    process_request_reply();

        // Handlers.
        void        handle_tx_send_comp(net::tx_op*);
        void        handle_rx_expiry(kernel::tqe*);
        uint64_t    handle_rx_dhcp(net::interface* intf, net::rx_page* p);
        void        handle_rx_dhcp_offer(const dhcp::eth_message* m);
        void        handle_rx_dhcp_ack(const dhcp::message* m);
        void        handle_rx_dhcp_nak(const dhcp::message* m);
        void        handle_arp_completion(kernel::wqe*);
        void        handle_t1_expiry(kernel::tqe*);
        void        handle_t2_expiry(kernel::tqe*);
        void        handle_lease_expiry(kernel::tqe*);

        // Constructor.
        client(eth::interface* intf);
    };
}

#endif /* __KERNEL_NET_DHCPC_H */
