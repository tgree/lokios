#include "tcp.h"
#include "header.h"
#include "traits.h"
#include "net/interface.h"
#include "net/ip/ip.h"
#include "mm/slab.h"
#include "kernel/spinlock.h"

using namespace tcp;

static void tcp_reply_cb(net::tx_op* op);

static kernel::spinlock op_lock;
static kernel::slab     op_slab(sizeof(tcp::tx_op));

static tcp::tx_op*
alloc_reply(net::interface* intf, net::rx_page* p)
{
    tcp::tx_op* top;
    with (op_lock)
    {
        top = op_slab.alloc<tcp::tx_op>();
    }

    auto* sh = p->payload_cast<tcp::ipv4_tcp_headers*>();
    top->hdrs.init(SIP{sh->ip.dst_ip},DIP{sh->ip.src_ip},
                   SPORT{sh->tcp.dst_port},DPORT{sh->tcp.src_port});

    uint8_t llh[16];
    size_t llsize = intf->format_ll_reply(p,llh,sizeof(llh));
    memcpy(top->llhdr + sizeof(top->llhdr) - llsize,llh,llsize);

    top->cb            = kernel::func_delegate(tcp_reply_cb);
    top->flags         = NTX_FLAG_INSERT_IP_CSUM | NTX_FLAG_INSERT_TCP_CSUM;
    top->nalps         = 1;
    top->alps[0].paddr = kernel::virt_to_phys(&top->hdrs.ip) - llsize;
    top->alps[0].len   = llsize + sizeof(ipv4::header) + sizeof(tcp::header);

    return top;
}

static void
free_reply(tcp::tx_op* top)
{
    with (op_lock)
    {
        op_slab.free(top);
    }
}

static void
tcp_reply_cb(net::tx_op* op)
{
    free_reply(static_cast<tcp::tx_op*>(op));
}

template<typename ...Args>
static inline void
post(net::interface* intf, net::rx_page* p, Args... args)
{
    auto* r = alloc_reply(intf,p);
    r->hdrs.format(args...);
    intf->post_tx_frame(r);
}

uint64_t
tcp::handle_rx_ipv4_tcp_frame(net::interface* intf, net::rx_page* p)
{
    // Sanity on the packet.
    auto* h = p->payload_cast<ipv4_tcp_headers*>();
    if (h->ip.total_len < sizeof(ipv4_tcp_headers))
        return 0;

    // We only handle unicast TCP packets.
    if (h->ip.dst_ip != intf->ip_addr)
        return 0;

    // Look for an existing tcp::socket first.
    uint16_t dst_port = h->tcp.dst_port;
    auto sid          = socket_id{h->ip.src_ip,h->tcp.src_port,dst_port};
    try
    {
        return intf->tcp_sockets[sid].handle_rx_ipv4_tcp_frame(p);
    }
    catch (hash::no_such_key_exception&)
    {
    }

    // No socket; look for a tcp::listener.
    try
    {
        auto& l = intf->tcp_listeners[dst_port];
        if (l.should_accept(&h->tcp))
        {
            auto& s        = intf->tcp_sockets.emplace(sid,intf,p);
            uint64_t flags = s.handle_rx_ipv4_tcp_frame(p);
            if (s.state != tcp::socket::TCP_LISTEN)
                l.socket_accepted(&s);
            else
                intf->tcp_sockets.erase_value(&s);
            return flags;
        }
    }
    catch (hash::no_such_key_exception&)
    {
    }

    // Either there was no listener or the listener chose to reject the SYN.
    // Treat this as though we were in the TCP CLOSED state.
    if (h->tcp.rst)
        return 0;
    if (h->tcp.ack)
        post(intf,p,SEQ{h->tcp.ack_num},CTL{FRST});
    else
        post(intf,p,ACK{h->tcp.seq_num + h->segment_len()},CTL{FRST|FACK});

    return 0;
}
