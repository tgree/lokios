#ifndef __KERNEL_NET_INTERFACE_H
#define __KERNEL_NET_INTERFACE_H

#include "tcp/socket.h"
#include "hdr/compiler.h"
#include <stdarg.h>

namespace net
{
    struct interface;

    // Handler for inbound frames.
    typedef void (*frame_handler)(interface* intf, void* cookie,
                                  net::rx_page* p);

    // Method delegate for inbound frames.
    template<typename T, void (T::*Handler)(interface* intf, net::rx_page*)>
    struct _frame_delegate
    {
        static void handler(interface* intf, void* cookie, net::rx_page* p)
        {
            (((T*)cookie)->*Handler)(intf,p);
        }
    };
#define frame_delegate(fn) \
    net::_frame_delegate< \
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

        // Register UDP frame handlers.
        inline  void    register_udp_handler(uint16_t port, void* cookie,
                                             net::frame_handler handler)
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
        virtual void    activate();
                void    refill_rx_pages();

        // Post a receive page.  The entire page is available for the driver's
        // use even if the chip doesn't support a large enough MTU to use it
        // efficiently.
        virtual void    post_rx_pages(kernel::klist<net::rx_page>& pages) = 0;

        // Constructor.
        interface(size_t tx_qlen, size_t rx_qlen);
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_INTERFACE_H */
