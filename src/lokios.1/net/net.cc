#include "net.h"
#include "interface.h"

kernel::kdlist<net::interface> net::interfaces;

void
net::register_interface(net::interface* intf)
{
    net::interfaces.push_back(&intf->link);
    net::wapi_node.register_child(&intf->wapi_node);
}

void
net::deregister_interface(net::interface* intf)
{
    intf->wapi_node.deregister();
    intf->link.unlink();
}

static void
net_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // GET /net
    rsp->printf("{\r\n"
                "    \"interfaces\" : [");
    for (auto& intf : klist_elems(net::interfaces,link))
    {
        rsp->printf("\r\n        { "
                    "\"name\" : \"net%zu\", "
                    "\"ip\" : \"%u.%u.%u.%u\", "
                    "\"tx_mtu\" : \"%u\", "
                    "\"rx_mtu\" : \"%u\" },",
                    intf.id,intf.ip_addr[0],intf.ip_addr[1],intf.ip_addr[2],
                    intf.ip_addr[3],intf.tx_mtu,intf.rx_mtu);
    }
    rsp->ks.shrink();
    rsp->printf("\r\n    ]\r\n}\r\n");
}

wapi::node net::wapi_node(func_delegate(net_request),METHOD_GET_MASK,"net");

void
net::init_net()
{
    wapi::root_node.register_child(&net::wapi_node);
}
