#include "response.h"
#include "net/tcp/socket.h"

void
http::response::printf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
    ks.vprintf(fmt,ap);
    va_end(ap);
}

void
http::response::headerf(const char* fmt, ...)
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

tcp::send_op*
http::response::send(tcp::socket* s)
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

tcp::send_op*
http::response::send_error(tcp::socket* s)
{
    // The string contains the entire response.
    alps[0].paddr = kernel::virt_to_phys(ks.c_str());
    alps[0].len   = ks.strlen();
    return s->send(1,alps,send_comp_delegate);
}
