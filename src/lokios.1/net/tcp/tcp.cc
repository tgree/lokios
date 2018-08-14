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

static tcp::tx_op*
alloc_reply(net::interface* intf, net::rx_page* p)
{
    tcp::tx_op* top;
    with (op_lock)
    {
        top = op_slab.alloc<tcp::tx_op>();
    }
    top->format_reply(intf,p);
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

void
tcp::tx_op::format_reply(net::interface* intf, net::rx_page* p)
{
    auto* sh                 = p->payload_cast<tcp::ipv4_tcp_headers*>();
    hdrs.ip.version_ihl      = 0x45;
    hdrs.ip.dscp_ecn         = 0;
    hdrs.ip.total_len        = sizeof(ipv4::header) + sizeof(tcp::header);
    hdrs.ip.identification   = kernel::random();
    hdrs.ip.flags_fragoffset = 0x4000;
    hdrs.ip.ttl              = 64;
    hdrs.ip.proto            = tcp::net_traits::ip_proto;
    hdrs.ip.header_checksum  = 0;
    hdrs.ip.src_ip           = sh->ip.dst_ip;
    hdrs.ip.dst_ip           = sh->ip.src_ip;
    hdrs.tcp.src_port        = sh->tcp.dst_port;
    hdrs.tcp.dst_port        = sh->tcp.src_port;
    hdrs.tcp.seq_num         = 0;
    hdrs.tcp.ack_num         = 0;
    hdrs.tcp.flags_offset    = 0x5000;
    hdrs.tcp.window_size     = 0;
    hdrs.tcp.checksum        = 0;
    hdrs.tcp.urgent_pointer  = 0;

    uint8_t llh[16];
    size_t llsize = intf->format_ll_reply(p,llh,sizeof(llh));
    memcpy(llhdr + sizeof(llhdr) - llsize,llh,llsize);

    cb            = kernel::func_delegate(tcp_reply_cb);
    flags         = NTX_FLAG_INSERT_IP_CSUM | NTX_FLAG_INSERT_TCP_CSUM;
    nalps         = 1;
    alps[0].paddr = kernel::virt_to_phys(&hdrs.ip) - llsize;
    alps[0].len   = llsize + sizeof(ipv4::header) + sizeof(tcp::header);
}

void
tcp::tx_op::format_rst(uint32_t seq_num)
{
    hdrs.tcp.seq_num = seq_num;
    hdrs.tcp.rst     = 1;
}

void
tcp::tx_op::format_rst_ack(uint32_t ack_num)
{
    hdrs.tcp.seq_num = 0;
    hdrs.tcp.ack_num = ack_num;
    hdrs.tcp.ack     = 1;
    hdrs.tcp.rst     = 1;
}

void
tcp::tx_op::format_ack(uint32_t seq_num, uint32_t ack_num, uint16_t window_size)
{
    hdrs.tcp.seq_num     = seq_num;
    hdrs.tcp.ack_num     = ack_num;
    hdrs.tcp.ack         = 1;
    hdrs.tcp.window_size = window_size;
}

static void
post_rst(net::interface* intf, net::rx_page* p, uint32_t seq_num)
{
    auto* r = alloc_reply(intf,p);
    r->format_rst(seq_num);
    intf->post_tx_frame(r);
}

static void
post_rst_ack(net::interface* intf, net::rx_page* p, uint32_t ack_num)
{
    auto* r = alloc_reply(intf,p);
    r->format_rst_ack(ack_num);
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
        post_rst(intf,p,h->tcp.ack_num);
    else
    {
        uint32_t seg_len = h->ip.total_len - sizeof(h->ip) - 4*h->tcp.offset +
                           h->tcp.syn + h->tcp.fin;
        post_rst_ack(intf,p,h->tcp.seq_num + seg_len);
    }

    return 0;
}
