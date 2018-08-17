#ifndef __KERNEL_NET_TCP_SOCKET_H
#define __KERNEL_NET_TCP_SOCKET_H

#include "tcp.h"
#include "net/ip/ip.h"
#include "mm/slab.h"
#include "k++/delegate.h"

// Socket parameters.
#define MAX_RX_WINDOW           0x01FFFFFF  // 32M
#define RX_WINDOW_SHIFT         9

namespace net
{
    struct interface;
}

namespace tcp
{
    struct socket;

    struct listener
    {
        // Decide whether or not to accept a SYN connection request.
        kernel::delegate<bool(const header* syn)>   should_accept;

        // Called when a new connection has been accepted.  The client should
        // fill out the observer delegate on the socket.
        kernel::delegate<void(socket*)>             socket_accepted;
    };
    typedef typeof_field(listener,should_accept)   should_accept_delegate;
    typedef typeof_field(listener,socket_accepted) socket_accepted_delegate;

    // Async socket notifications.
    struct socket_observer
    {
        virtual void socket_established(socket*) = 0;
        virtual void socket_readable(socket*) = 0;
        virtual void socket_reset(socket*) = 0;
    };

#define OPTION_SND_MSS_PRESENT          (1<<0)
#define OPTION_SND_WND_SHIFT_PRESENT    (1<<1)
    struct parsed_options
    {
        uint32_t    flags;
        uint32_t    snd_mss;
        uint8_t     snd_wnd_shift;
    };

    struct socket
    {
        enum tcp_state
        {
            TCP_CLOSED,
            TCP_LISTEN,
            TCP_SYN_SENT,
            TCP_SYN_RECVD,
        };

        net::interface*                 intf;
        tcp_state                       state;
        tcp_state                       prev_state;
        uint8_t                         llhdr[16];
        tcp::ipv4_tcp_headers           hdrs;
        const size_t                    llsize;
        const ipv4::addr                remote_ip;
        const uint16_t                  local_port;
        const uint16_t                  remote_port;
        socket_observer*                observer;

        // Send queue.
        kernel::slab                    tx_ops_slab;
        kernel::klist<tcp::tx_op>       posted_ops;
        kernel::timer_entry             retransmit_wqe;

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

        // Allocate send ops.
        tcp::tx_op* alloc_tx_op();
        void free_tx_op(tcp::tx_op* top);

        // Post send ops.
        void    post_op(tcp::tx_op* top);
        void    post_syn(uint32_t seq_num, uint16_t mss, size_t window_size);
        void    post_rst(uint32_t seq_num);
        void    post_ack(uint32_t seq_num, uint32_t ack_num,
                         size_t window_size, uint8_t window_shift);
        void    send_complete(net::tx_op* nop);

        // Handlers.
        void        handle_retransmit_expiry(kernel::timer_entry* wqe);
        uint64_t    handle_rx_ipv4_tcp_frame(net::rx_page* p);
        uint64_t    handle_listen_syn_recvd(net::rx_page* p);

        // Helpers.
        void        dump_socket();
        int         parse_options(ipv4_tcp_headers* h, parsed_options* opts);

        // Passive open.
        socket(net::interface* intf, net::rx_page* p);

        // Active open.
        socket(net::interface* intf, ipv4::addr remote_ip, uint16_t local_port,
               uint16_t remote_port, const void* llhdr, size_t llsize,
               socket_observer* observer);
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
