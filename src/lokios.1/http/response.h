#ifndef __KERNEL_HTTP_RESPONSE_H
#define __KERNEL_HTTP_RESPONSE_H

#include "kern/kstring.h"

namespace http
{
    struct string_response
    {
        kernel::string  ks;
        kernel::dma_alp alp;

        void printf(const char* fmt, ...)
        {
            va_list ap;
            va_start(ap,fmt);
            ks.vprintf(fmt,ap);
            va_end(ap);
        }

        tcp::send_op* send(tcp::socket* s,
                           kernel::delegate<void(tcp::send_op*)> cb =
                            func_delegate(tcp::socket::send_noop_cb))
        {
            alp.paddr = kernel::virt_to_phys(ks.c_str());
            alp.len   = ks.strlen();
            return s->send(1,&alp,cb);
        }
    };
}

#endif /* __KERNEL_HTTP_RESPONSE_H */
