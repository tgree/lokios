#ifndef __KERNEL_HTTP_RESPONSE_H
#define __KERNEL_HTTP_RESPONSE_H

#include "kern/kstring.h"
#include "k++/delegate.h"

namespace tcp
{
    struct socket;
    struct send_op;
}

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
            send_comp_delegate = func_delegate(send_noop_cb);

        // Internal use.
        kernel::dma_alp alps[2] = {{0,0}, {0,0}};

        void reset(kernel::delegate<void(tcp::send_op*)> send_cb =
                   func_delegate(send_noop_cb));

        void printf(const char* fmt, ...);
        void headerf(const char* fmt, ...);

        tcp::send_op* send(tcp::socket* s);
        tcp::send_op* send_error(tcp::socket* s);

        static void send_noop_cb(tcp::send_op*) {}
    };
}

#endif /* __KERNEL_HTTP_RESPONSE_H */
