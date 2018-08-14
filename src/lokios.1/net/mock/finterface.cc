#include "finterface.h"
#include "kernel/kassert.h"

using kernel::_kassert;

net::finterface::finterface(ipv4::addr _ip_addr):
    net::interface::interface(16,16)
{
    ip_addr = _ip_addr;
}

net::finterface::~finterface()
{
    kassert(posted_ops.empty());
    while (!posted_pages.empty())
    {
        auto* p = klist_front(posted_pages,link);
        posted_pages.pop_front();
        free_rx_page(p);
    }
}

size_t
net::finterface::format_ll_reply(net::rx_page* p, void* reply_payload)
{
    return 0;
}

size_t
net::finterface::format_arp_broadcast(void* arp_payload)
{
    kernel::panic("format_arp_broadcast not supported");
}

void
net::finterface::post_tx_frame(net::tx_op* op)
{
    posted_ops.push_back(&op->link);
}

void
net::finterface::post_rx_pages(kernel::klist<net::rx_page>& pages)
{
    posted_pages.append(pages);
}

void
net::finterface::dump_arp_table()
{
    kernel::panic("dump_arp_table not supported");
}

net::tx_op*
net::finterface::pop_tx_op()
{
    kassert(!posted_ops.empty());
    auto* op = klist_front(posted_ops,link);
    posted_ops.pop_front();
    return op;
}

net::rx_page*
net::finterface::pop_rx_page()
{
    kassert(!posted_pages.empty());
    auto* p = klist_front(posted_pages,link);
    posted_pages.pop_front();
    --rx_posted_count;
    return p;
}

uint32_t
net::finterface::handle_rx_page(net::rx_page* p)
{
    uint64_t flags = handle_rx_ipv4_frame(p);
    if (!(flags & NRX_FLAG_NO_DELETE))
        free_rx_page(p);
    refill_rx_pages();
    return flags;
}
