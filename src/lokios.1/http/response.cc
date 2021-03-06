#include "response.h"
#include "net/tcp/socket.h"

void
http::response::reset(kernel::delegate<void(tcp::send_op*)> send_cb)
{
    send_comp_delegate = send_cb;
    alps[0].paddr      = 0;
    alps[0].len        = 0;
    alps[1].paddr      = 0;
    alps[1].len        = 0;
    ks.clear();
}

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
    alps[0].paddr = kernel::virt_to_phys(ks.c_str() + alps[1].len);
    alps[0].len   = ks.strlen() - alps[1].len;
    if (alps[1].len)
        alps[1].paddr = kernel::virt_to_phys(ks.c_str());
    return s->send(alps[1].len ? 2 : 1,alps,send_comp_delegate);
}
