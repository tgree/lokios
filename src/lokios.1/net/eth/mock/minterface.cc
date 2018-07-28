#include "../interface.h"
#include "../traits.h"
#include "net/arp/arp.h"
#include "net/dhcp/dhcpc.h"
#include <tmock/tmock.h>

eth::interface::interface(const eth::addr& hw_mac, size_t tx_qlen,
    size_t rx_qlen):
        hw_mac(hw_mac),
        ip_addr{0,0,0,0},
        tx_qlen(tx_qlen),
        rx_qlen(rx_qlen),
        rx_posted_count(0),
        phy(NULL)
{
    dhcpc = new dhcp::client(this);
    arpc_ipv4 = new arp::service<eth::net_traits,ipv4::net_traits>(this);
}

eth::interface::~interface()
{
}

void
eth::interface::activate()
{
    mock("eth::interface::activate");
}

void
eth::interface::refill_rx_pages()
{
    mock("eth::interface::refill_rx_pages");
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
eth::interface::handle_tx_completion(net::tx_op* op)
{
    mock("eth::interface::handle_tx_completion",op);
}

void
eth::interface::handle_rx_pages(kernel::klist<net::rx_page>& pages)
{
    mock("eth::interface::handle_rx_pages",&pages);
}

void
eth::interface::handle_rx_ipv4_frame(net::rx_page* p)
{
    mock("eth::interface::handle_rx_ipv4_frame",p);
}

void
eth::interface::handle_rx_ipv4_udp_frame(net::rx_page* p)
{
    mock("eth::interface::handle_rx_ipv4_udp_frame",p);
}

void
eth::interface::handle_rx_arp_frame(net::rx_page* p)
{
    mock("eth::interface::handle_rx_arp_frame",p);
}
