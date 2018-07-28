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

        // Interface memory.
        interface_mem*      intf_mem;

        // Emit log messages.
                void                 intf_vdbg(const char* fmt, va_list ap);
        inline  void __PRINTF__(2,3) intf_dbg(const char* fmt, ...)
        {
            va_list ap;
            va_start(ap,fmt);
            intf_vdbg(fmt,ap);
            va_end(ap);
        }

        // Constructor.
        interface();
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_INTERFACE_H */
