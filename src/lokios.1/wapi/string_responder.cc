#include "string_responder.h"

static kernel::slab string_responder_slab(
                        MAX(sizeof(wapi::string_responder_node),
                            sizeof(http::string_response)));

wapi::string_responder_node*
wapi::register_string_responder(wapi::node* parent,
    wapi::string_responder_delegate handler, void* cookie, uint64_t method_mask,
    const char* fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
    auto* sr = string_responder_slab.alloc<string_responder_node>(
                    handler,cookie,method_mask,fmt,ap);
    va_end(ap);

    parent->register_child(sr);

    return sr;
}

wapi::string_responder_node::string_responder_node(
    string_responder_delegate handler, void* cookie, uint64_t method_mask,
    const char* fmt, va_list ap):
        wapi::node(fmt,ap),
        handler(handler),
        cookie(cookie),
        method_mask(method_mask)
{
}

wapi::string_responder_node::string_responder_node(
    string_responder_delegate handler, void* cookie, uint64_t method_mask,
    const char* fmt, ...):
        handler(handler),
        cookie(cookie),
        method_mask(method_mask)
{
    va_list ap;
    va_start(ap,fmt);
    name.vprintf(fmt,ap);
    va_end(ap);
}

void
wapi::string_responder_node::handle_request(const http::request* req,
    tcp::socket* s)
{
    auto* rsp = string_responder_slab.alloc<http::string_response>();
    if (method_mask & (1<<req->method))
        handler(req,rsp,cookie);
    else
    {
        rsp->printf("HTTP/1.1 405 Method Not Allowed\r\n");
        rsp->printf("Allow: ");
        uint64_t methods = method_mask;
        for (size_t i=0; i<http::NUM_METHODS; ++i)
        {
            bool supported = (methods & 1);
            methods      >>= 1;
            if (supported)
            {
                rsp->printf("%s",http::get_method_name((http::method)i));
                if (methods)
                    rsp->printf(", ");
            }
        }
        rsp->printf("\r\n\r\n");
    }
    rsp->send(s,method_delegate(send_cb))->cookie = rsp;
    s->close_send();
}

void
wapi::string_responder_node::send_cb(tcp::send_op* sop)
{
    string_responder_slab.free((http::string_response*)sop->cookie);
}
