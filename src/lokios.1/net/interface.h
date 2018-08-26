#ifndef __KERNEL_NET_INTERFACE_H
#define __KERNEL_NET_INTERFACE_H

#include "cmd_sock.h"
#include "tcp/socket.h"
#include "k++/hash_table.h"
#include "k++/ring.h"
#include "k++/kmath.h"
#include <stdarg.h>

#define NUM_EPHEMERAL_PORTS 2048
KASSERT(kernel::is_pow2(NUM_EPHEMERAL_PORTS));

#define EPHEMERAL_ORDER \
    kernel::ulog2(NUM_EPHEMERAL_PORTS*sizeof(uint16_t)/PAGE_SIZE)
KASSERT((PAGE_SIZE << EPHEMERAL_ORDER) == sizeof(uint16_t)*NUM_EPHEMERAL_PORTS);

#define FIRST_EPHEMERAL_PORT (65536-NUM_EPHEMERAL_PORTS)

namespace net
{
    struct interface;

    // Method delegate for inbound frames.  Returns a NRX_FLAG_ value.
    typedef kernel::delegate<uint64_t(interface*,rx_page*)> udp_frame_handler;

    struct interface
    {
        // The netX id number.
        const size_t        id;

        // Size of the hardware transmit and receive queues.
        const size_t        tx_qlen;
        const size_t        rx_qlen;
        size_t              rx_posted_count;

        // IP address assigned by software.
        ipv4::addr          ip_addr;

        // Link-layer MTUs.  These are the maximum frame payload size following
        // the link-layer header and not including any link-layer footer.  For
        // Ethernet, the typical value is 1500 which is the largest allowable
        // client payload in an Ethernet frame.  Subtracting the 40 bytes for
        // the IP and TCP headers (which from Ethernet's perspective are part
        // of the client payload) yields the typical TCP MSS of 1460 bytes.
        uint16_t            tx_mtu;
        uint16_t            rx_mtu;

        // UDP.
        hash::table<uint16_t,udp_frame_handler> udp_sockets;

        // TCP.
        hash::table<uint16_t,tcp::listener>     tcp_listeners;
        hash::table<tcp::socket_id,tcp::socket> tcp_sockets;
        kernel::buddy_block<EPHEMERAL_ORDER>    tcp_ephemeral_ports_mem;
        kernel::ring<uint16_t>                  tcp_ephemeral_ports;

        // Default test sockets.
        cmd_sock_listener   cmd_listener;

        // Emit log messages.
                void                 intf_vdbg(const char* fmt, va_list ap);
        inline  void __PRINTF__(2,3) intf_dbg(const char* fmt, ...)
        {
            va_list ap;
            va_start(ap,fmt);
            intf_vdbg(fmt,ap);
            va_end(ap);
        }

        // Format the link-layer header of a reply packet.  It's assumed that in
        // the rx_page parameter we've received some sort of packet (of
        // unspecified type, but it includes a link-layer header) that we need
        // to format a reply to.  It's also assumed that the rx_page you are
        // replying to is in the link-layer-stripped form.  Returns the actual
        // length of the link-layer header.
        virtual size_t  format_ll_reply(net::rx_page* p, void* ll_hdr,
                                        size_t ll_hdr_len) = 0;

        // Format an ARP request's link-layer header.  This will be a broadcast
        // packet of some sort.  This works similarly to format_ll_reply where
        // you pass a pointer to the arp payload and there is sufficient space
        // preceding it to hold a link-layer header.
        virtual size_t  format_arp_broadcast(void* arp_payload) = 0;

        // UDP
                void    udp_listen(uint16_t port, udp_frame_handler h);
                void    udp_ignore(uint16_t port);

        // TCP.
                void    tcp_listen(uint16_t port,
                                   tcp::socket_accepted_delegate ad,
                                   tcp::should_accept_delegate sad =
                                    kernel::func_delegate(tcp_always_accept));
                void    tcp_ignore(uint16_t port);
        static  bool    tcp_always_accept(const tcp::header*) {return true;}
        tcp::socket*    tcp_connect(ipv4::addr remote_ip, uint16_t remote_port,
                                    tcp::socket_observer* observer,
                                    uint16_t local_port = 0);
                void    tcp_unlink(tcp::socket* s);
                void    tcp_delete(tcp::socket* s);

        // Rx page management.
        inline  rx_page*    alloc_rx_page() {return new rx_page;}
        inline  void        free_rx_page(net::rx_page* p) {delete p;}

        // Activate the interface.
        virtual void    activate();
                void    refill_rx_pages();

        // Transmit a frame.
        virtual void    post_tx_frame(net::tx_op* op) = 0;

        // Post a receive page.  The entire page is available for the driver's
        // use even if the chip doesn't support a large enough MTU to use it
        // efficiently.
        virtual void    post_rx_pages(kernel::klist<net::rx_page>& pages) = 0;

        // Handlers.  The return value is a set of flags indicating what should
        // be done with the page after returning.  The normal behavior is to
        // auto-delete the page, but if it is to go onto a client queue or to
        // be deleted by someone else then the NRX_FLAG_NO_DELETE flag should
        // be set in the return code.
                void        handle_tx_completion(net::tx_op* op);
                void        handle_delayed_completion(kernel::timer_entry* wqe);
                uint64_t    handle_rx_ipv4_frame(net::rx_page* p);
                uint64_t    handle_rx_ipv4_udp_frame(net::rx_page* p);

        // Helpers.
        virtual void    dump_arp_table() = 0;

        // Constructor.
        interface(size_t tx_qlen, size_t rx_qlen, uint16_t tx_mtu,
                  uint16_t rx_mtu);
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_INTERFACE_H */
