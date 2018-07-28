#ifndef __KERNEL_NET_INTERFACE_H
#define __KERNEL_NET_INTERFACE_H

#include "hdr/compiler.h"
#include <stddef.h>
#include <stdarg.h>

namespace net
{
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
