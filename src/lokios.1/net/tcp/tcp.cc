#include "tcp.h"
#include "header.h"
#include "traits.h"
#include "net/interface.h"
#include "net/ip/ip.h"
#include "mm/slab.h"
#include "kernel/spinlock.h"
#include "k++/random.h"

static kernel::spinlock op_lock;
static kernel::slab     op_slab(sizeof(tcp::tx_op));

static void
tcp_reply_cb(net::tx_op* op)
{
    auto* top = static_cast<tcp::tx_op*>(op);
    with (op_lock)
    {
        op_slab.free(top);
    }
}

tcp::tx_op*
tcp::alloc_reply(net::interface* intf, net::rx_page* p)
{
    tcp::tx_op* r;
    with (op_lock)
    {
        r = op_slab.alloc<tcp::tx_op>();
    }

    auto* sh                    = p->payload_cast<tcp::ipv4_tcp_headers*>();
    r->hdrs.ip.version_ihl      = 0x45;
    r->hdrs.ip.dscp_ecn         = 0;
    r->hdrs.ip.total_len        = sizeof(ipv4::header) + sizeof(tcp::header);
    r->hdrs.ip.identification   = kernel::random();
    r->hdrs.ip.flags_fragoffset = 0x4000;
    r->hdrs.ip.ttl              = 64;
    r->hdrs.ip.proto            = tcp::net_traits::ip_proto;
    r->hdrs.ip.header_checksum  = 0;
    r->hdrs.ip.src_ip           = sh->ip.dst_ip;
    r->hdrs.ip.dst_ip           = sh->ip.src_ip;
    r->hdrs.tcp.src_port        = sh->tcp.dst_port;
    r->hdrs.tcp.dst_port        = sh->tcp.src_port;
    r->hdrs.tcp.seq_num         = 0;
    r->hdrs.tcp.ack_num         = 0;
    r->hdrs.tcp.flags_offset    = 0x5000;
    r->hdrs.tcp.window_size     = 0;
    r->hdrs.tcp.checksum        = 0;
    r->hdrs.tcp.urgent_pointer  = 0;

    size_t llsize    = intf->format_ll_reply(p,&r->hdrs.ip);
    r->cb            = kernel::func_delegate(tcp_reply_cb);
    r->flags         = NTX_FLAG_INSERT_IP_CSUM | NTX_FLAG_INSERT_TCP_CSUM;
    r->nalps         = 1;
    r->alps[0].paddr = kernel::virt_to_phys(&r->hdrs.ip) - llsize;
    r->alps[0].len   = llsize + sizeof(ipv4::header) + sizeof(tcp::header);

    return r;
}

void
tcp::post_rst(net::interface* intf, net::rx_page* p, uint32_t seq_num)
{
    auto* r             = tcp::alloc_reply(intf,p);
    r->hdrs.tcp.seq_num = seq_num;
    r->hdrs.tcp.rst     = 1;
    intf->post_tx_frame(r);
}

void
tcp::post_rst_ack(net::interface* intf, net::rx_page* p, uint32_t ack_num)
{
    auto* r             = tcp::alloc_reply(intf,p);
    r->hdrs.tcp.seq_num = 0;
    r->hdrs.tcp.ack_num = ack_num;
    r->hdrs.tcp.ack     = 1;
    r->hdrs.tcp.rst     = 1;
    intf->post_tx_frame(r);
}

void
tcp::post_ack(net::interface* intf, net::rx_page* p, uint32_t seq_num,
    uint32_t ack_num, uint16_t window_size)
{
    auto* r                 = tcp::alloc_reply(intf,p);
    r->hdrs.tcp.seq_num     = seq_num;
    r->hdrs.tcp.ack_num     = ack_num;
    r->hdrs.tcp.ack         = 1;
    r->hdrs.tcp.window_size = window_size;
    intf->post_tx_frame(r);
}

uint64_t
tcp::handle_rx_ipv4_tcp_frame(net::interface* intf, net::rx_page* p)
{
    // Sanity on the packet.
    if (p->pay_len < sizeof(ipv4_tcp_headers))
        return 0;

    // We only handle unicast TCP packets.
    auto* h = p->payload_cast<ipv4_tcp_headers*>();
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
            auto& s        = intf->tcp_sockets.emplace(sid,intf,dst_port);
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
        post_rst(intf,p,h->tcp.ack_num);
    else
    {
        size_t seg_len = h->ip.total_len - sizeof(h->ip) - 4*h->tcp.offset;
        post_rst_ack(intf,p,h->tcp.seq_num + seg_len);
    }

    return 0;
}
