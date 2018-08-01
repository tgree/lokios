#include "interface.h"
#include "tcp/traits.h"
#include "tcp/tcp.h"
#include "udp/udp.h"
#include "kernel/console.h"
#include "mm/vm.h"
#include "mm/slab.h"
#include "k++/kmath.h"

using kernel::_kassert;

static kernel::spinlock ids_lock;
static uint32_t free_ids = 0xFFFFFFFF;

static size_t
alloc_id()
{
    with (ids_lock)
    {
        kassert(free_ids != 0);
        size_t id = kernel::ffs(free_ids);
        free_ids &= ~(1<<id);
        return id;
    }
}

static void
free_id(size_t id)
{
    with (ids_lock)
    {
        free_ids |= (1<<id);
    }
}

net::interface::interface(size_t tx_qlen, size_t rx_qlen):
    id(alloc_id()),
    tx_qlen(tx_qlen),
    rx_qlen(rx_qlen),
    rx_posted_count(0),
    intf_mem((interface_mem*)(MM_ETH_BEGIN + id*MM_ETH_STRIDE)),
    ip_addr{0,0,0,0}
{
    kernel::vmmap(intf_mem,sizeof(*intf_mem));
    memset(intf_mem,0,sizeof(*intf_mem));
    new(intf_mem) interface_mem();
}

net::interface::~interface()
{
    intf_mem->~interface_mem();
    kernel::vmunmap(intf_mem,sizeof(*intf_mem));
    free_id(id);
}

void
net::interface::intf_vdbg(const char* fmt, va_list ap)
{
    kernel::console::p2printf(fmt,ap,"net%zu: ",id);
}

void
net::interface::udp_listen(uint16_t port, udp_frame_handler h)
{
    udp_sockets.emplace(port,h);
}

void
net::interface::udp_ignore(uint16_t port)
{
    udp_sockets.erase(port);
}

void
net::interface::tcp_listen(uint16_t port,
    tcp::socket_accepted_delegate ad,
    tcp::should_accept_delegate sad)
{
    tcp_listeners.emplace(port,tcp::listener{sad,ad});
}

void
net::interface::tcp_delete(tcp::socket* s)
{
    intf_dbg("deleting socket\n");
    delete s;
}

void
net::interface::cmd_socket_accepted(tcp::socket* s)
{
    s->rx_readable = method_delegate(cmd_socket_readable);
}

void
net::interface::cmd_socket_readable(tcp::socket* s)
{
    char buffer[16];
    memset(buffer,0,sizeof(buffer));
    while (s->rx_avail_bytes)
    {
        uint32_t len = kernel::min(s->rx_avail_bytes,sizeof(buffer)-1);
        s->read(buffer,len);
        if (!strcmp(buffer,"arp\r\n"))
            dump_arp_table();
    }
    kassert(s->rx_pages.empty());
}

void
net::interface::activate()
{
    // Post receive buffers.
    refill_rx_pages();

    // Add a handler.
    tcp_listen(12345,method_delegate(cmd_socket_accepted));
}

void
net::interface::refill_rx_pages()
{
    kernel::klist<net::rx_page> pages;
    size_t n = rx_qlen - rx_posted_count;
    for (size_t i=0; i<n; ++i)
    {
        auto* p = new net::rx_page;
        pages.push_back(&p->link);
    }
    rx_posted_count = rx_qlen;
    post_rx_pages(pages);
}

void
net::interface::handle_rx_ipv4_frame(net::rx_page* p)
{
    auto* iph = p->payload_cast<ipv4::header*>();
    if (iph->dst_ip != ip_addr && iph->dst_ip != ipv4::broadcast_addr)
        return;

    switch (iph->proto)
    {
        case tcp::net_traits::ip_proto:
            if (iph->dst_ip == ip_addr)
                tcp::handle_rx_ipv4_tcp_frame(this,p);
        break;

        case udp::net_traits::ip_proto:
            handle_rx_ipv4_udp_frame(p);
        break;
    }
}

void
net::interface::handle_rx_ipv4_udp_frame(net::rx_page* p)
{
    auto* iph = p->payload_cast<ipv4::header*>();
    auto* uh  = (udp::header*)(iph+1);

    udp_frame_handler* ufh;
    try
    {
        ufh = &udp_sockets[uh->dst_port];
    }
    catch (hash::no_such_key_exception&)
    {
        return;
    }

    (*ufh)(this,p);
}

