#include "../interface.h"
#include "k++/kmath.h"
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

net::interface::interface(size_t tx_qlen, size_t rx_qlen, uint16_t tx_mtu,
    uint16_t rx_mtu):
        id(alloc_id()),
        tx_qlen(tx_qlen),
        rx_qlen(rx_qlen),
        rx_posted_count(0),
        ip_addr{0,0,0,0},
        tx_mtu(tx_mtu),
        rx_mtu(rx_mtu),
        tcp_ephemeral_ports((uint16_t*)tcp_ephemeral_ports_mem.addr,
                            tcp_ephemeral_ports_mem.len/sizeof(uint16_t))
{
}

net::interface::~interface()
{
    free_id(id);
}

void
net::interface::intf_vdbg(const char* fmt, va_list ap)
{
    mock("net::interface::intf_vdbg",fmt,ap);
}

void
net::interface::udp_listen(uint16_t port, udp_frame_handler h)
{
    mock("net::interface::udp_listen",port,&h);
}

void
net::interface::udp_ignore(uint16_t port)
{
    mock("net::interface::udp_ignore",port);
}

void
net::interface::refill_rx_pages()
{
    mock("net::interface::refill_rx_pages");
}

void
net::interface::notify_activated()
{
    mock("net::interface::notify_activated");
}

uint64_t
net::interface::handle_rx_ipv4_frame(net::rx_page* p)
{
    return (uint64_t)mock("net::interface::handle_rx_ipv4_frame",p);
}

uint64_t
net::interface::handle_rx_ipv4_udp_frame(net::rx_page* p)
{
    return (uint64_t)mock("net::interface::handle_rx_ipv4_udp_frame",p);
}

