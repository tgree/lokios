#include "socket.h"
#include "traits.h"
#include "net/interface.h"
#include "k++/random.h"

using kernel::_kassert;

tcp::socket::socket(net::interface* intf, net::rx_page* p):
    intf(intf),
    state(TCP_LISTEN),
    prev_state(TCP_CLOSED),
    llsize(intf->format_ll_reply(p,&hdrs.ll,sizeof(hdrs.ll))),
    tx_ops_slab(sizeof(tcp::tx_op))
{
    kassert(llsize <= sizeof(hdrs.ll));
    uint8_t llh[sizeof(hdrs.ll)];
    memcpy(llh,hdrs.ll,sizeof(hdrs.ll));
    memset(hdrs.ll,0xDD,sizeof(hdrs.ll));
    memcpy(hdrs.ll + sizeof(hdrs.ll) - llsize,llh,llsize);

    auto* sh                 = p->payload_cast<tcp::ipv4_tcp_headers*>();
    hdrs.ip.version_ihl      = 0x45;
    hdrs.ip.dscp_ecn         = 0;
    hdrs.ip.total_len        = sizeof(ipv4::header) + sizeof(tcp::header);
    hdrs.ip.identification   = 0;
    hdrs.ip.flags_fragoffset = 0x4000;
    hdrs.ip.ttl              = 64;
    hdrs.ip.proto            = tcp::net_traits::ip_proto;
    hdrs.ip.header_checksum  = 0;
    hdrs.ip.src_ip           = intf->ip_addr;
    hdrs.ip.dst_ip           = sh->ip.src_ip;
    hdrs.tcp.src_port        = sh->tcp.dst_port;
    hdrs.tcp.dst_port        = sh->tcp.src_port;
    hdrs.tcp.seq_num         = 0;
    hdrs.tcp.ack_num         = 0;
    hdrs.tcp.flags_offset    = 0x5000;
    hdrs.tcp.window_size     = 0;
    hdrs.tcp.checksum        = 0;
    hdrs.tcp.urgent_pointer  = 0;
}

tcp::tx_op*
tcp::socket::alloc_tx_op()
{
    auto* top                   = tx_ops_slab.alloc<tx_op>();
    top->hdrs                   = hdrs;
    top->hdrs.ip.identification = kernel::random();
    top->flags                  = NTX_FLAG_INSERT_IP_CSUM |
                                  NTX_FLAG_INSERT_TCP_CSUM;
    top->nalps                  = 1;
    top->alps[0].paddr          = kernel::virt_to_phys(&top->hdrs.ip) - llsize;
    top->alps[0].len            = llsize + sizeof(ipv4::header) +
                                  sizeof(tcp::header);
    return top;
}

void
tcp::socket::free_tx_op(tcp::tx_op* top)
{
    tx_ops_slab.free(top);
}

uint64_t
tcp::socket::handle_rx_ipv4_tcp_frame(net::rx_page* p)
{
    return 0;
}
