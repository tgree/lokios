#include "../interface.h"
#include "../traits.h"
#include "net/arp/arp.h"
#include "net/dhcp/dhcpc.h"
#include <tmock/tmock.h>

eth::interface::interface(const eth::addr& hw_mac, size_t tx_qlen,
    size_t rx_qlen, uint16_t tx_mtu, uint16_t rx_mtu):
        net::interface(tx_qlen,rx_qlen,tx_mtu,rx_mtu),
        hw_mac(hw_mac),
        phy(NULL),
        arp_node(method_delegate(handle_arp_wapi_request),METHOD_GET_MASK,"arp")
{
    dhcpc = new dhcp::client(this);
    arpc_ipv4 = new arp::service<eth::net_traits,ipv4::net_traits>(this);
}

eth::interface::~interface()
{
}

size_t
eth::interface::format_ll_reply(net::rx_page* p, void* ll_hdr,
    size_t ll_hdr_len)
{
    return (size_t)mock("eth::interface::format_ll_reply",p,ll_hdr,ll_hdr_len);
}

size_t
eth::interface::format_arp_broadcast(void* arp_payload)
{
    return (size_t)mock("eth::interface::format_arp_broadcast",arp_payload);
}

void
eth::interface::handle_dhcp_success()
{
    mock("eth::interface::handle_dhcp_success");
}

void
eth::interface::handle_dhcp_failure()
{
    mock("eth::interface::handle_dhcp_failure");
}

void
eth::interface::handle_rx_pages(kernel::klist<net::rx_page>& pages)
{
    mock("eth::interface::handle_rx_pages",&pages);
}

void
eth::interface::handle_rx_arp_frame(net::rx_page* p)
{
    mock("eth::interface::handle_rx_arp_frame",p);
}

void
eth::interface::handle_arp_wapi_request(wapi::node* node, http::request* req,
    json::object* obj, http::response* rsp)
{
    mock("eth::interface::handle_arp_wapi_request",node,req,obj,rsp);
}

void
eth::interface::handle_wapi_request(wapi::node* node, http::request* req,
    json::object* obj, http::response* rsp)
{
    mock("eth::interface::handle_wapi_request",node,req,obj,rsp);
}
