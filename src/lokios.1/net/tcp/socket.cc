#include "socket.h"
#include "traits.h"
#include "net/interface.h"

tcp::socket::socket(net::interface* intf, uint16_t port):
    intf(intf),
    state(TCP_LISTEN),
    prev_state(TCP_CLOSED)
{
#if 0
    hdrs.ll.dst_mac          = eth::net_traits::zero_addr;
    hdrs.ll.src_mac          = intf->hw_mac;
    hdrs.ll.ether_type       = ipv4::net_traits::ether_type;
#endif
    hdrs.ip.version_ihl      = 0x45;
    hdrs.ip.dscp_ecn         = 0;
    hdrs.ip.total_len        = sizeof(ipv4::header) + sizeof(tcp::header);
    hdrs.ip.identification   = 0;
    hdrs.ip.flags_fragoffset = 0x4000;
    hdrs.ip.ttl              = 64;
    hdrs.ip.proto            = tcp::net_traits::ip_proto;
    hdrs.ip.header_checksum  = 0;
    hdrs.ip.src_ip           = intf->ip_addr;
    hdrs.ip.dst_ip           = ipv4::addr{0,0,0,0};
    hdrs.tcp.src_port        = port;
    hdrs.tcp.dst_port        = 0;
    hdrs.tcp.seq_num         = 0;
    hdrs.tcp.ack_num         = 0;
    hdrs.tcp.flags_offset    = 0;
    hdrs.tcp.window_size     = 0;
    hdrs.tcp.checksum        = 0;
    hdrs.tcp.urgent_pointer  = 0;
}

uint64_t
tcp::socket::handle_rx_ipv4_tcp_frame(net::rx_page* p)
{
    return 0;
}
