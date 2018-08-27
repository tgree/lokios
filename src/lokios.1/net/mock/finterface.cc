#include "finterface.h"
#include "kernel/kassert.h"

using kernel::_kassert;

net::finterface::finterface(ipv4::addr _ip_addr, uint16_t tx_mtu,
    uint16_t rx_mtu):
        net::interface::interface(16,16,tx_mtu,rx_mtu)
{
    ip_addr = _ip_addr;
    refill_rx_pages();
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
net::finterface::format_ll_reply(net::rx_page* p, void* ll_hdr,
    size_t ll_hdr_len)
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

uint64_t
net::finterface::handle_rx_page(net::rx_page* p)
{
    uint64_t flags = handle_rx_ipv4_frame(p);
    if (!(flags & NRX_FLAG_NO_DELETE))
        free_rx_page(p);
    refill_rx_pages();
    return flags;
}

static void
copy_top_to_rxp(const net::tx_op* op, net::rx_page* rp)
{
    rp->pay_offset = 0;
    rp->pay_len    = 0;
    uint8_t* p     = rp->payload;
    for (size_t i=0; i<op->nalps; ++i)
    {
        auto* alp = &op->alps[i];
        memcpy(p,kernel::phys_to_virt(alp->paddr),alp->len);
        p           += alp->len;
        rp->pay_len += alp->len;
    }
}

net::fpipe::fpipe(finterface* intf0, finterface* intf1)
{
    intfs[0] = intf0;
    intfs[1] = intf1;
}

size_t
net::fpipe::process_queues()
{
    size_t npackets = 0;

    kernel::klist<net::tx_op> posted_ops[2];
    posted_ops[0].append(intfs[0]->posted_ops);
    posted_ops[1].append(intfs[1]->posted_ops);

    while (!posted_ops[0].empty() || !posted_ops[1].empty())
    {
        if (!posted_ops[0].empty())
        {
            auto* rp = intfs[1]->pop_rx_page();
            auto* op = klist_front(posted_ops[0],link);
            posted_ops[0].pop_front();
            copy_top_to_rxp(op,rp);
            intfs[0]->handle_tx_completion(op);
            intfs[1]->handle_rx_page(rp);
            ++npackets;
        }

        if (!posted_ops[1].empty())
        {
            auto* rp = intfs[0]->pop_rx_page();
            auto* op = klist_front(posted_ops[1],link);
            posted_ops[1].pop_front();
            copy_top_to_rxp(op,rp);
            intfs[1]->handle_tx_completion(op);
            intfs[0]->handle_rx_page(rp);
            ++npackets;
        }
    }

    return npackets;
}

size_t
net::fpipe::drop_queues()
{
    size_t npackets = 0;

    for (size_t i=0; i<2; ++i)
    {
        kernel::klist<net::tx_op> posted_ops;
        posted_ops.append(intfs[i]->posted_ops);

        while (!posted_ops.empty())
        {
            auto* op = klist_front(posted_ops,link);
            posted_ops.pop_front();
            intfs[i]->handle_tx_completion(op);
            ++npackets;
        }
    }

    return npackets;
}
