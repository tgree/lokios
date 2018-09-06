#include "wapi.h"
#include "net/interface.h"
#include "net/wka.h"

struct wapi_net_observer : public net::observer
{
    virtual void intf_activated(net::interface* intf) override;
};

struct wapi_connection : public tcp::socket_observer
{
    http::request   request;
    http::response  response;

    virtual void    socket_established(tcp::socket* s);
    virtual void    socket_readable(tcp::socket* s);
    virtual void    socket_recv_closed(tcp::socket* s);
    virtual void    socket_closed(tcp::socket* s);
    virtual void    socket_reset(tcp::socket* s);
};

static kernel::slab connection_slab(sizeof(wapi_connection));
wapi_net_observer wapi_observer;

static void
socket_accepted(tcp::socket* s)
{
    s->observer = connection_slab.alloc<wapi_connection>();
}

void
wapi_net_observer::intf_activated(net::interface* intf)
{
    intf->intf_dbg("starting wapi port\n");
    intf->tcp_listen(TCP_LOKIOS_WAPI_PORT,4096,func_delegate(socket_accepted));
}

void
wapi_connection::socket_established(tcp::socket*)
{
}

void
wapi_connection::socket_readable(tcp::socket* s) try
{
    if (request.is_done())
        return;

    try
    {
        while (s->rx_avail_bytes && !request.is_done())
        {
            auto* p     = klist_front(s->rx_pages,link);
            char* start = (char*)p->payload + p->client_offset;
            size_t n    = request.parse(start,p->client_len);
            s->skip(n);
        }
    }
    catch (http::exception)
    {
        throw wapi::bad_request_exception();
    }

    if (!request.is_done())
        return;

    s->dbg("%s %s HTTP/%u.%u\n",
           http::get_method_name(request.method),
           request.request_target,
           (request.version >> 4),
           (request.version & 0xF));

    auto* n = wapi::find_node_for_path(request.request_target);
    if (!n)
        throw wapi::not_found_exception();
    if (!(n->method_mask & (1<<request.method)))
        throw wapi::method_not_allowed_exception(n->method_mask);
    try
    {
        if (request.method == http::METHOD_GET ||
            request.body.empty() ||
            strcmp(request.headers["Content-Type"],"application/json"))
        {
            n->handler(n,&request,NULL,&response);
        }
        else
        {
            json::object obj;
            json::parse_object(request.body.c_str(),&obj);
            n->handler(n,&request,&obj,&response);
        }
    }
    catch (json::exception)
    {
        throw wapi::bad_request_exception();
    }
    catch (hash::no_such_key_exception)
    {
        throw wapi::bad_request_exception();
    }

    response.send(s);
    s->close_send();
}
catch (wapi::not_found_exception)
{
    response.printf("HTTP/1.1 404 Not Found\r\n"
                    "Content-Length: 15\r\n"
                    "\r\n"
                    "404 Not Found\r\n");
    response.send_error(s);
    s->close_send();
}
catch (wapi::bad_request_exception)
{
    response.printf("HTTP/1.1 400 Bad Request\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n");
    response.send_error(s);
    s->close_send();
}
catch (wapi::method_not_allowed_exception& e)
{
    response.printf("HTTP/1.1 405 Method Not Allowed\r\n"
                    "Content-Length: 0\r\n"
                    "Allow: ");
    uint64_t methods = e.method_mask;
    for (size_t i=0; i<http::NUM_METHODS; ++i)
    {
        bool supported = (methods & 1);
        methods      >>= 1;
        if (supported)
        {
            response.printf("%s",http::get_method_name((http::method)i));
            if (methods)
                response.printf(", ");
        }
    }
    response.printf("\r\n\r\n");
    response.send_error(s);
    s->close_send();
}

void
wapi_connection::socket_recv_closed(tcp::socket* s)
{
}

void
wapi_connection::socket_closed(tcp::socket* s)
{
    s->intf->tcp_delete(s);
    connection_slab.free(this);
}

void
wapi_connection::socket_reset(tcp::socket* s)
{
    s->dbg("wapi socket reset\n");
}
