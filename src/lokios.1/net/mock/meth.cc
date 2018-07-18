#include "../eth.h"
#include "../dhcpc.h"
#include "../arp.h"
#include <tmock/tmock.h>

static uint32_t free_ids = 0xFFFFFFFF;

static size_t
alloc_id()
{
    kernel::kassert(free_ids != 0);
    size_t id = kernel::ffs(free_ids);
    free_ids &= ~(1<<id);
    return id;
}

static void
free_id(size_t id)
{
    free_ids |= (1<<id);
}

eth::interface::interface(const eth::addr& hw_mac, size_t tx_qlen,
    size_t rx_qlen):
	id(alloc_id()),
        intf_mem((eth::interface_mem*)malloc(sizeof(*intf_mem))),
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
    free_id(id);
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
eth::interface::handle_tx_completion(eth::tx_op* op)
{
    mock("eth::interface::handle_tx_completion",op);
}

void
eth::interface::handle_rx_pages(kernel::klist<rx_page>& pages)
{
    mock("eth::interface::handle_rx_pages",&pages);
}

void
eth::interface::handle_rx_ipv4_frame(rx_page* p)
{
    mock("eth::interface::handle_rx_ipv4_frame",p);
}

void
eth::interface::handle_rx_ipv4_udp_frame(rx_page* p)
{
    mock("eth::interface::handle_rx_ipv4_udp_frame",p);
}

void
eth::interface::handle_rx_arp_frame(rx_page* p)
{
    mock("eth::interface::handle_rx_arp_frame",p);
}
