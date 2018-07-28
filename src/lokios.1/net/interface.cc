#include "interface.h"
#include "tcp/traits.h"
#include "udp/udp.h"
#include "kernel/console.h"
#include "mm/vm.h"
#include "mm/slab.h"
#include "k++/kmath.h"

using kernel::_kassert;

static kernel::spinlock ids_lock;
static uint32_t free_ids = 0xFFFFFFFF;

static kernel::spinlock slab_lock;
static kernel::slab tcp_slab(sizeof(tcp::listener));

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
net::interface::tcp_listen(uint16_t port, tcp::connection_filter f)
{
    kassert(!intf_mem->tcp_listeners[port]);

    tcp::listener* l;
    with (slab_lock)
    {
        l = tcp_slab.alloc<tcp::listener>();
    }

    l->intf                       = this;
    l->port                       = port;
    l->filter_delegate            = f;
    intf_mem->tcp_listeners[port] = l;
}

void
net::interface::activate()
{
    // Post receive buffers.
    refill_rx_pages();
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
    auto* iph = (ipv4::header*)(p->payload + p->pay_offset);
    if (iph->dst_ip != ip_addr && iph->dst_ip != ipv4::broadcast_addr)
        return;

    switch (iph->proto)
    {
        case tcp::net_traits::ip_proto: handle_rx_ipv4_tcp_frame(p);    break;
        case udp::net_traits::ip_proto: handle_rx_ipv4_udp_frame(p);    break;
    }
}

void
net::interface::handle_rx_ipv4_tcp_frame(net::rx_page* p)
{
    auto* iph         = (ipv4::header*)(p->payload + p->pay_offset);
    auto* th          = (tcp::header*)(iph+1);
    uint16_t dst_port = th->dst_port;

    auto* l = intf_mem->tcp_listeners[th->dst_port];
    if (!l)
    {
        intf_dbg("ipv4 tcp frame received and dropped on port %u\n",dst_port);
        return;
    }

    int err = l->filter_delegate(th);
    if (err)
    {
        intf_dbg("ipv4 tcp frame received and rejected on port %u with err "
                 "%d\n",dst_port,err);
        return;
    }

    intf_dbg("ipv4 tcp frame accepted on port %u\n",dst_port);
    delete p;
}

void
net::interface::handle_rx_ipv4_udp_frame(net::rx_page* p)
{
    auto* iph = (ipv4::header*)(p->payload + p->pay_offset);
    auto* uh  = (udp::header*)(iph+1);
    auto* ufh = &intf_mem->udp_frame_handlers[uh->dst_port];
    if (ufh->handler)
        (*ufh->handler)(this,ufh->cookie,p);
}

