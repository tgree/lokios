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
            void    handle_request();
            void    response_send_complete(tcp::send_op* sop);
    virtual void    socket_recv_closed(tcp::socket* s);
    virtual void    socket_closed(tcp::socket* s);
    virtual void    socket_reset(tcp::socket* s);

    wapi_connection();
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

wapi_connection::wapi_connection()
{
    response.send_comp_delegate = method_delegate(response_send_complete);
}

void
wapi_connection::socket_established(tcp::socket*)
{
}

void
wapi_connection::socket_readable(tcp::socket* s)
{
    // If we already have a completed request, then defer until we've
    // finished handling it.
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
    catch (const http::exception&)
    {
        // This indicates some error while trying to parse the request; i.e. it
        // is malformed in some way.  We cannot recover the stream at this
        // point so we force closure.
        response.send_comp_delegate =
            func_delegate(http::response::send_noop_cb);
        response.headerf("HTTP/1.1 400 Bad Request\r\n"
                         "Content-Length: 0\r\n"
                         "\r\n");
        response.send(s)->cookie = s;
        s->close_send();
        return;
    }

    // If we haven't finished parsing (more to come from the remote client),
    // then bail out now.
    if (!request.is_done())
        return;

    // Generate the response.  At this point we have properly parsed the
    // request, so the stream is always recoverable.  handle_request will
    // generate whatever type of response we should send and then we'll send
    // it.
    handle_request();
    response.send(s)->cookie = s;
    if (!request.should_keepalive())
        s->close_send();
}

void
wapi_connection::handle_request() try
{
    // Try to find a node to handle this request.
    auto* n = wapi::find_node_for_path(request.request_target);
    if (!n)
        throw wapi::not_found_exception();
    if (!(n->method_mask & (1<<request.method)))
        throw wapi::method_not_allowed_exception(n->method_mask);

    // Dispatch it.
    if (request.is_json_request())
    {
        json::object obj;
        json::parse_object(request.body.c_str(),&obj);
        n->handler(n,&request,&obj,&response);
    }
    else
        n->handler(n,&request,NULL,&response);

    // Success path.
    response.headerf("HTTP/1.1 200 OK\r\n"
                     "Content-Type: application/json\r\n"
                     "Content-Length: %zu\r\n"
                     "\r\n",
                     response.ks.strlen());
}
catch (const http::missing_content_type_exception&)
{
    // This indicates that there was a request body but no Content-Type
    // field to tell us how to interpret it.  Recoverable.
    response.headerf("HTTP/1.1 400 Bad Request\r\n"
                     "Content-Length: 0\r\n"
                     "\r\n");
}
catch (const json::exception&)
{
    // We couldn't parse the JSON body.  The stream is recoverable even if
    // the JSON is gibberish.
    response.headerf("HTTP/1.1 400 Bad Request\r\n"
                     "Content-Length: 0\r\n"
                     "\r\n");
}
catch (const wapi::not_found_exception&)
{
    // Node not found.  Recoverable.
    response.headerf("HTTP/1.1 404 Not Found\r\n"
                     "Content-Length: 0\r\n"
                     "\r\n");
}
catch (const wapi::bad_request_exception&)
{
    // Something was wrong at the wapi level.  Examples include setting the
    // root '/' node json 'state' field to an unrecognized value or issuing
    // a POST without an expected JSON body.  Recoverable.
    response.headerf("HTTP/1.1 400 Bad Request\r\n"
                     "Content-Length: 0\r\n"
                     "\r\n");
}
catch (const wapi::method_not_allowed_exception& e)
{
    // This node doesn't support the requested method.  Recoverable.
    response.headerf("HTTP/1.1 405 Method Not Allowed\r\n"
                     "Content-Length: 0\r\n"
                     "Allow: ");
    uint64_t methods = e.method_mask;
    for (size_t i=0; i<http::NUM_METHODS; ++i)
    {
        bool supported = (methods & 1);
        methods      >>= 1;
        if (supported)
        {
            response.headerf("%s",http::get_method_name((http::method)i));
            if (methods)
                response.headerf(", ");
        }
    }
    response.headerf("\r\n\r\n");
}

void
wapi_connection::response_send_complete(tcp::send_op* sop)
{
    request.reset();
    response.reset(method_delegate(response_send_complete));
    
    auto* s = (tcp::socket*)sop->cookie;
    if (s->rx_avail_bytes)
        socket_readable(s);
}

void
wapi_connection::socket_recv_closed(tcp::socket* s)
{
    if (s->in_sendable_state())
        s->close_send();
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
