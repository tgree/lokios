#ifndef __KERNEL_HTTP_RESPONSE_H
#define __KERNEL_HTTP_RESPONSE_H

#include "kern/kstring.h"
#include "net/tcp/socket.h"

namespace http
{
    struct response
    {
        // If you have a non-json response, change the content_type field.
        const char* content_type = "application/json";

        // This is your response body.
        kernel::string  ks;

        // Usually you don't care when the response has been ACK'd, but for a
        // very few commands such as stopping the kernel we do - that's when
        // we'll take the deferred action.
        kernel::delegate<void(tcp::send_op*)>
            send_comp_delegate = func_delegate(tcp::socket::send_noop_cb);

        // Internal use.
        kernel::dma_alp alps[2] = {{0,0}, {0,0}};

        void printf(const char* fmt, ...) __PRINTF__(2,3)
        {
            va_list ap;
            va_start(ap,fmt);
            ks.vprintf(fmt,ap);
            va_end(ap);
        }

        void headerf(const char* fmt, ...) __PRINTF__(2,3)
        {
            if (!alps[1].paddr)
            {
                alps[1].len   = ks.strlen();
                alps[1].paddr = -1;
            }

            va_list ap;
            va_start(ap,fmt);
            ks.vprintf(fmt,ap);
            va_end(ap);
        }

        tcp::send_op* send(tcp::socket* s)
        {
            // The string contains a response body, everything was OK.
            headerf("HTTP/1.1 200 OK\r\n"
                    "Content-Type: %s\r\n"
                    "Content-Length: %zu\r\n"
                    "\r\n",
                    content_type,
                    ks.strlen());
            alps[0].paddr = kernel::virt_to_phys(ks.c_str() + alps[1].len);
            alps[0].len   = ks.strlen() - alps[1].len;
            if (alps[1].len)
                alps[1].paddr = kernel::virt_to_phys(ks.c_str());
            return s->send(alps[1].len ? 2 : 1,alps,send_comp_delegate);
        }

        tcp::send_op* send_error(tcp::socket* s)
        {
            // The string contains the entire response.
            alps[0].paddr = kernel::virt_to_phys(ks.c_str());
            alps[0].len   = ks.strlen();
            return s->send(1,alps,send_comp_delegate);
        }
    };
}

#endif /* __KERNEL_HTTP_RESPONSE_H */
