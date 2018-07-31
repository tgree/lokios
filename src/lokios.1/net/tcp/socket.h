#ifndef __KERNEL_NET_TCP_SOCKET_H
#define __KERNEL_NET_TCP_SOCKET_H

#include "header.h"
#include "rx_queue.h"
#include "net/net.h"
#include "net/ip/ip.h"
#include "k++/delegate.h"
#include "mm/slab.h"

namespace net
{
    struct interface;
}

namespace tcp
{
    struct socket;

    typedef kernel::delegate<socket*(const header* syn)> alloc_delegate;
    struct listener
    {
        net::interface*         intf;
        uint16_t                port;
        alloc_delegate          socket_allocator;
    };

    struct socket
    {
        enum tcp_state
        {
            TCP_CLOSED,
            TCP_LISTEN,
            TCP_SYN_RECVD,
            TCP_ESTABLISHED,
            TCP_CLOSE_WAIT,
        };

        kernel::kdlink                  link;
        net::interface*                 intf;
        tcp::ll_ipv4_tcp_headers        hdrs;
        tcp_state                       state;
        tcp_state                       prev_state;
        tcp::rx_queue*                  rq;

        // Send sequence variables.
        uint32_t                        snd_una;
        uint32_t                        snd_nxt;
        uint32_t                        snd_wnd;
        uint16_t                        snd_mss;
        uint8_t                         snd_wnd_shift;
        uint32_t                        iss;

        // Receive sequence variables.
        uint32_t                        rcv_nxt;
        uint32_t                        rcv_wnd;
        uint8_t                         rcv_wnd_shift;
        uint16_t                        rcv_mss;
        uint32_t                        irs;

        // Other TCB variables.
        uint32_t                        dup_ack;
        uint32_t                        ss_thresh;
        uint32_t                        cwnd;
        uint32_t                        exp_boff;

        // Handlers.
        void    handle_rx_ipv4_tcp_frame(net::rx_page* p);
        void    handle_listen_syn_recvd(net::rx_page* p);
        void    handle_established_segment_recvd(net::rx_page* p);

        // Helpers.
        void    dump_socket();
        void    window_update(uint32_t seg_len);
        void    process_fin(net::rx_page* p);

        // Passive open.
        socket(net::interface* intf, uint16_t port, tcp::rx_queue* rq);
    };

    struct socket_id
    {
        ipv4::addr  remote_ip;
        uint16_t    remote_port;
        uint16_t    local_port;
    };
    constexpr bool operator==(const socket_id& lhs, const socket_id& rhs)
    {
        return lhs.remote_ip   == rhs.remote_ip &&
               lhs.remote_port == rhs.remote_port &&
               lhs.local_port  == rhs.local_port;
    }
}

#include "k++/hash.h"
namespace hash
{
    template<>
    inline size_t compute(const tcp::socket_id& k)
    {
        return hash::compute(k.remote_ip) + k.remote_port + k.local_port;
    }
}

#endif /* __KERNEL_NET_TCP_SOCKET_H */
