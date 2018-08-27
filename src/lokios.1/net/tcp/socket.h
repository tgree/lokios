/*
 * Main TCP socket class.  A note on TCP variables:
 *  
 *  SND_UNA - this is the first sequence number that hasn't been acknowledged
 *            yet.  Note that if SND_UNA == SND_NXT then we haven't even sent
 *            SND_UNA yet.
 *  SND_NXT - this is our first unsent sequence number.
 *  SND_WND - this is the current size of the send window we can use (as
 *            advertised by the remote guy).  The send window range is:
 *                  [SND_UNA, SND_UNA + SND_WND) 
 *
 *  RCV_NXT - this is the next unreceived sequence number - we are expecting to
 *            see this one next.
 *  RCV_WND - this is the current size of our receive window (which we
 *            advertise to the remote guy).  The receive window range is:
 *                  [RCV_NXT, RCV_NXT + RCV_WND)
 *
 *  Header.ACK_NUM - this is the remote guy's RCV_NXT value.  He is ACKing
 *            everything up to but not including ACK_NUM.  Upon receipt of an
 *            ACK_NUM, the remote guy is explicitly ACKing the range:
 *                  [SND_UNA, ACK_NUM)
 *            ..which may be empty if SND_UNA == ACK_NUM.
 *            Upon receipt of an ACK that we should advance our SND_UNA to
 *            ACK_NUM (assuming ACK_NUM overlaps with our unacknowledged range
 *            - duplicate ACKs don't!).
 *
 *            More: the valid range for Header.ACK_NUM is [SND_UNA, SND_NXT].
 *            Anything outside that range has either already been acknowledged
 *            or is from the future.
 */
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
        virtual void socket_closed(socket*) = 0;
        virtual void socket_reset(socket*) = 0;
    };

#define SEND_OP_FLAG_SYN        (1<<0)   // SYN before any payload alps.
#define SEND_OP_FLAG_FIN        (1<<1)   // FIN after any payload alps.
#define SEND_OP_FLAG_SET_ACK    (1<<2)   // Set the ACK bit on the SYN.
#define SEND_OP_FLAG_SET_SCALE  (1<<3)   // Set window scaling option on SYN.
    struct send_op
    {
        kernel::kdlink                      link;
        kernel::delegate<void(send_op*)>    cb;
        uint64_t                            flags;
        size_t                              refcount;
        size_t                              nalps;
        size_t                              unacked_alp_index;
        size_t                              unsent_alp_index;
        size_t                              unacked_alp_offset;
        size_t                              unsent_alp_offset;
        kernel::dma_alp*                    alps;

        // Marks as many bytes ACKed as possible and returns the remainder of
        // ack_len.
        uint32_t    mark_acked(uint32_t ack_len);

        // Determines if the op has been fully acked.
        bool        is_fully_acked() const;
    };

    struct socket
    {
        enum tcp_state
        {
            TCP_CLOSED,
            TCP_LISTEN,
            TCP_SYN_SENT,
            TCP_SYN_SENT_ACKED_WAIT_SYN,
            TCP_SYN_SENT_SYN_RECVD_WAIT_ACK,
            TCP_SYN_RECVD,
            TCP_ESTABLISHED,
            TCP_CLOSE_WAIT,
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
        // Send ops transition between queues as follows:
        // - unsent_send_ops - these ops have bytes that haven't even been
        //                     posted to the chip yet.
        // - sent_send_ops   - the bytes for these ops have all been posted to
        //                     the chip but some are still unacknowledged
        // - acked_send_ops  - the bytes for these ops have all been posted and
        //                     acked but the op still has a non-zero refcount
        //                     (missing send completions from the chip).
        kernel::slab                    tx_ops_slab;
        kernel::slab                    send_ops_slab;
        kernel::klist<tcp::tx_op>       posted_ops;
        kernel::kdlist<tcp::send_op>    unsent_send_ops;
        kernel::kdlist<tcp::send_op>    sent_send_ops;
        kernel::kdlist<tcp::send_op>    acked_send_ops;
        kernel::timer_entry             retransmit_wqe;

        // Receive queue.
        kernel::klist<net::rx_page>     rx_pages;
        size_t                          rx_avail_bytes;

        // Options we received from the remote guy.
        parsed_options                  rx_opts;

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

        // Allocate send ops.
        tcp::tx_op* alloc_tx_op(tcp::send_op* sop = NULL);
        void free_tx_op(tcp::tx_op* top);

        // Post send ops.
        void    post_op(tcp::tx_op* top);
        void    post_retransmittable_op(tcp::tx_op* top);
        void    post_rst(uint32_t seq_num);
        void    post_ack(uint32_t seq_num, uint32_t ack_num,
                         size_t window_size, uint8_t window_shift);
        void    send_complete(net::tx_op* nop);
        void    send_complete_arm_retransmit(net::tx_op* nop);

        // Send data.  This is async; the alps and data must remain valid until
        // after the callback is invoked.
        tcp::send_op*   send(size_t nalps, kernel::dma_alp* alps,
                             kernel::delegate<void(send_op*)> cb,
                             uint64_t flags = 0);
        tcp::tx_op*     make_one_packet(tcp::send_op* sop);
        void            process_send_queue();
        void            process_ack(uint32_t ack_num);

        // Receive data.
        void    rx_append(net::rx_page* p);
        void    read(void* dst, uint32_t len);

        // Send completion for SYN packets.
        void        syn_send_op_cb(tcp::send_op* top);

        // Handlers.
        void        handle_retransmit_expiry(kernel::timer_entry* wqe);
        uint64_t    handle_rx_ipv4_tcp_frame(net::rx_page* p);
        uint64_t    handle_synchronized_segment_recvd(net::rx_page* p);

        // Helpers.
        void        process_header_synchronized(const ipv4_tcp_headers* h);
        uint64_t    process_payload_synchronized(net::rx_page* p);
        void        process_options(parsed_options opts);
        void        dump_socket();

        // Passive open.
        socket(net::interface* intf, net::rx_page* p, parsed_options rx_opts);

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
