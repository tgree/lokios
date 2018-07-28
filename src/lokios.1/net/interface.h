#ifndef __KERNEL_NET_INTERFACE_H
#define __KERNEL_NET_INTERFACE_H

#include "net.h"
#include "hdr/compiler.h"
#include <stddef.h>
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

    struct interface
    {
        // The netX id number.
        const size_t        id;

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
