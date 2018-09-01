#include "../socket.h"
#include "net/interface.h"
#include <tmock/tmock.h>

tcp::socket::socket(net::interface* intf, net::rx_page* p,
    parsed_options rx_opts, uint32_t rcv_wnd):
        intf(intf),
        state(TCP_CLOSED),
        llsize(intf->format_ll_reply(p,&llhdr,sizeof(llhdr))),
        remote_ip(p->payload_cast<tcp::ipv4_tcp_headers*>()->ip.src_ip),
        local_port(p->payload_cast<tcp::ipv4_tcp_headers*>()->tcp.dst_port),
        remote_port(p->payload_cast<tcp::ipv4_tcp_headers*>()->tcp.src_port),
        observer(NULL),
        tx_ops_slab(sizeof(tcp::tx_op)),
        send_ops_slab(sizeof(tcp::send_op)),
        rx_avail_bytes(0),
        rx_opts(rx_opts),
        rcv_wnd(rcv_wnd),
        last_ack_ack_num(0),
        last_ack_wnd_size(0)
{
}

tcp::socket::socket(net::interface* intf, ipv4::addr remote_ip,
    uint16_t local_port, uint16_t remote_port, const void* llh,
    size_t llsize, socket_observer* observer, uint32_t rcv_wnd):
        intf(intf),
        state(TCP_CLOSED),
        llsize(llsize),
        remote_ip(remote_ip),
        local_port(local_port),
        remote_port(remote_port),
        observer(observer),
        tx_ops_slab(sizeof(tcp::tx_op)),
        send_ops_slab(sizeof(tcp::send_op)),
        rx_avail_bytes(0),
        rcv_wnd(rcv_wnd),
        last_ack_ack_num(0),
        last_ack_wnd_size(0)
{
}
