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

static tcp::tx_op*
tcp_alloc_reply(net::interface* intf, net::rx_page* p)
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

static void
post_rst(net::interface* intf, net::rx_page* p, uint32_t seq_num)
{
    auto* r             = tcp_alloc_reply(intf,p);
    r->hdrs.tcp.seq_num = seq_num;
    r->hdrs.tcp.rst     = 1;
    intf->post_tx_frame(r);
}

static void
post_rst_ack(net::interface* intf, net::rx_page* p, uint32_t ack_num)
{
    auto* r             = tcp_alloc_reply(intf,p);
    r->hdrs.tcp.seq_num = 0;
    r->hdrs.tcp.ack_num = ack_num;
    r->hdrs.tcp.ack     = 1;
    r->hdrs.tcp.rst     = 1;
    intf->post_tx_frame(r);
}

uint64_t
tcp::handle_rx_ipv4_tcp_frame(net::interface* intf, net::rx_page* p)
{
    // CLOSED state handling since we don't have any kind of a stack yet.
    auto* h = p->payload_cast<ipv4_tcp_headers*>();
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
