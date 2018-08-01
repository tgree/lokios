#ifndef __KERNEL_NET_INTERFACE_H
#define __KERNEL_NET_INTERFACE_H

#include "tcp/socket.h"
#include "hdr/compiler.h"
#include <stdarg.h>

namespace net
{
    struct interface;

    // Method delegate for inbound frames.
    typedef kernel::delegate<void(interface*,rx_page*)> udp_frame_handler;

    // Data structure that gets mapped at the interface's reserved vaddr.
    struct interface_mem
    {
        udp_frame_handler   udp_frame_handlers[65536];
        tcp::listener*      tcp_listeners[65536];
    };

    struct interface
    {
        // The netX id number.
        const size_t        id;

        // Size of the hardware transmit and receive queues.
        const size_t        tx_qlen;
        const size_t        rx_qlen;
        size_t              rx_posted_count;

        // Interface memory.
        interface_mem*      intf_mem;

        // IP address assigned by software.
        ipv4::addr          ip_addr;

        // Emit log messages.
                void                 intf_vdbg(const char* fmt, va_list ap);
        inline  void __PRINTF__(2,3) intf_dbg(const char* fmt, ...)
        {
            va_list ap;
            va_start(ap,fmt);
            intf_vdbg(fmt,ap);
            va_end(ap);
        }

        // Format the link-layer part of a reply packet.  It's assumed that in
        // the rx_page parameter we've received some sort of packet (of
        // unspecified type, but it includes a link-layer header) that we need
        // to format a reply to.
        //
        // Furthermore, it's assumed that you have some sort of payload after
        // the link-layer header that you want to transmit; the address of that
        // payload is what's passed in via the payload pointer.
        //
        // For instance, for an ipv4 packet you might have something like:
        //
        //      struct packet
        //      {
        //          uint8_t         ll[64];
        //          ipv4::header    ip;
        //          ...
        //      };
        //
        // In this example, the payload pointer would be set to the address of
        // the ipv4 header and we've provided up to 64 bytes before the ip
        // header to hold the link-layer header.  The net::interface subclass
        // will prefix the payload with an appropriate-sized link-layer header
        // and return the header size (since it may not use the full 64 bytes -
        // i.e. Ethernet only uses 12 bytes).
        // 
        // Finally, it's assumed that the rx_page you are replying to is in the
        // link-layer-stripped form.
        virtual size_t  format_ll_reply(net::rx_page* p, 
                                        void* reply_payload) = 0;

        // Register UDP frame handlers.
        inline  void    register_udp_handler(uint16_t port, udp_frame_handler h)
        {
            auto& ufh = intf_mem->udp_frame_handlers[port];
            kernel::kassert(!ufh);
            ufh = h;
        }
        inline  void    deregister_udp_handler(uint16_t port)
        {
            auto& ufh = intf_mem->udp_frame_handlers[port];
            kernel::kassert(ufh);
            ufh.clear();
        }

        // TCP.
                void    tcp_listen(uint16_t port, tcp::connection_filter f);

        // Activate the interface.
        virtual void    activate();
                void    refill_rx_pages();

        // Transmit a frame.
        virtual void    post_tx_frame(net::tx_op* op) = 0;

        // Post a receive page.  The entire page is available for the driver's
        // use even if the chip doesn't support a large enough MTU to use it
        // efficiently.
        virtual void    post_rx_pages(kernel::klist<net::rx_page>& pages) = 0;

        // Handlers.
        inline  void    handle_tx_completion(net::tx_op* op) {op->cb(op);}
                void    handle_rx_ipv4_frame(net::rx_page* p);
                void    handle_rx_ipv4_udp_frame(net::rx_page* p);

        // Constructor.
        interface(size_t tx_qlen, size_t rx_qlen);
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_INTERFACE_H */
