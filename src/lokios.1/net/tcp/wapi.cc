#include "tcp.h"
#include "net/interface.h"

void
tcp::handle_wapi_request(wapi::node* n, http::request* req, json::object* obj,
    http::response* rsp)
{
    auto* intf = container_of(n,net::interface,tcp_node);
    rsp->printf("{\r\n"
                "    \"listeners\" : [");
    for (auto& e : intf->tcp_listeners)
        rsp->printf(" %u,",e.k);
    rsp->ks.shrink();
    rsp->printf(" ],\r\n"
                 "    \"sockets\" : [");
    for (auto& e : intf->tcp_sockets)
    {
        rsp->printf("\r\n        { \"id\" : \"%u:%u:%u.%u.%u.%u\", "
                    "\"state\" : \"%s\", "
                    "\"snd_mss\" : %u, "
                    "\"snd_wnd\" : %u, "
                    "\"snd_shift\" : %u, "
                    "\"rcv_mss\" : %u, "
                    "\"rcv_wnd\" : %u, "
                    "\"rcv_shift\" : %u, "
                    "\"rx_avail\" : %zu "
                    "},",
                    e.v.local_port,e.v.remote_port,
                    e.v.remote_ip[0],e.v.remote_ip[1],
                    e.v.remote_ip[2],e.v.remote_ip[3],
                    e.v.get_state_name(),
                    e.v.snd_mss,e.v.snd_wnd,e.v.snd_wnd_shift,
                    e.v.rcv_mss,e.v.rcv_wnd,e.v.rcv_wnd_shift,
                    e.v.rx_avail_bytes);
    }
    rsp->ks.shrink();
    rsp->printf("\r\n        ]\r\n"
                "}\r\n");
}
