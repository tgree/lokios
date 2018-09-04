#include "wapi.h"
#include "http/request.h"
#include "net/interface.h"
#include "net/wka.h"

struct wapi_net_observer : public net::observer
{
    virtual void intf_activated(net::interface* intf) override;
};

struct wapi_connection : public tcp::socket_observer
{
    http::request   request;

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

    while (s->rx_avail_bytes && !request.is_done())
    {
        auto* p        = klist_front(s->rx_pages,link);
        uint8_t* start = p->payload + p->client_offset;
        uint8_t* c     = start;
        for (size_t i=0; i<p->client_len; ++i)
        {
            if (!request.parse(*c++))
                break;
        }
        s->skip(c - start);
    }

    if (!request.is_done())
        return;

    s->dbg("%s %s HTTP/%u.%u\n",
           http::get_method_name(request.method),
           request.request_target.c_str(),
           (request.version >> 4),
           (request.version & 0xF));
#if 0
    for (auto& n : request.headers)
        s->dbg("%s: %s\n",n.k.c_str(),n.v.c_str());
#endif

    auto* n = wapi::find_node_for_path(request.request_target);
    if (n)
        n->handle_request(&request,s);
    else
        wapi::send_404_response(s);
}
catch (http::header_invalid_exception& e)
{
    s->dbg("invalid header\n");
}
catch (http::unrecognized_method_exception& e)
{
    s->dbg("unrecognized method\n");
}
catch (http::header_path_too_long_exception& e)
{
    s->dbg("path too long\n");
}
catch (http::header_key_too_long_exception& e)
{
    s->dbg("header field name too long\n");
}
catch (http::header_val_too_long_exception& e)
{
    s->dbg("header field value too long\n");
}
catch (http::exception& e)
{
    s->dbg("some sort of http exception\n");
}

void
wapi_connection::socket_recv_closed(tcp::socket* s)
{
    //s->dbg("wapi recv closed\n");
}

void
wapi_connection::socket_closed(tcp::socket* s)
{
    //s->dbg("wapi socket closed\n");
    s->intf->tcp_delete(s);
    connection_slab.free(this);
}

void
wapi_connection::socket_reset(tcp::socket* s)
{
    s->dbg("wapi socket reset\n");
}
