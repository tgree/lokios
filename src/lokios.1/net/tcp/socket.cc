#include "socket.h"
#include "traits.h"
#include "tcp.h"
#include "net/eth/traits.h"
#include "k++/random.h"
#include "kernel/console.h"

#define MAX_OPTIONS_LEN 16
#define MAX_RX_WINDOW   0x01FFFFFF  // 32M
#define RX_WINDOW_SHIFT 9

#define DEBUG_TRANSITIONS 1

#if DEBUG_TRANSITIONS
#define TRANSITION(s) \
    do                                                                   \
    {                                                                    \
        intf->intf_dbg("%s:%u: %u -> " #s "\n",__FILE__,__LINE__,state); \
        state = (s);                                                     \
    } while(0)
#else
#define TRANSITION(s) state = (s)
#endif

using kernel::_kassert;

tcp::socket::socket(net::interface* intf, uint16_t port, tcp::rx_queue* rq):
    intf(intf),
    state(TCP_LISTEN),
    prev_state(TCP_CLOSED),
    rq(rq)
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

    // TODO: I have no idea about these defaults.
    dup_ack   = 0;
    ss_thresh = 2;
    cwnd      = 0;
    exp_boff  = 1;
}

void
tcp::socket::handle_rx_ipv4_tcp_frame(net::rx_page* p)
{
    auto* h        = p->payload_cast<ipv4_tcp_headers*>();
    size_t seg_len = h->ip.total_len - sizeof(h->ip) - 4*h->tcp.offset;
    switch (state)
    {
        case TCP_LISTEN:
            if (h->tcp.rst)
            {
                intf->intf_dbg("packet had rst bit\n");
                break;
            }
            if (h->tcp.ack)
            {
                intf->intf_dbg("packet had ack bit\n");
                tcp::post_rst(intf,p,h->tcp.ack_num);
                break;
            }
            if (!h->tcp.syn)
            {
                intf->intf_dbg("packet didn't have syn bit\n");
                break;
            }

            handle_listen_syn_recvd(p);
            dump_socket();
        break;

        case TCP_SYN_RECVD:
            if (!seq_check(rcv_nxt,h->tcp.seq_num,seg_len,rcv_wnd))
            {
                if (!h->tcp.rst)
                {
                    tcp::post_ack(intf,p,snd_nxt,rcv_nxt,
                                  rcv_wnd >> rcv_wnd_shift);
                }
                return;
            }
            if (h->tcp.rst)
            {
                if (prev_state == TCP_LISTEN)
                    intf->intf_dbg("passive open failed, deleting socket\n");
                else
                    kernel::panic("active open failed - notify client!\n");
                intf->tcp_delete(this);
                return;
            }
            if (h->tcp.syn)
            {
                // We're just going to close the socket; the packet could get
                // dropped.  It seems like this is unnecessary.
                tcp::post_rst(intf,p,snd_nxt);
                intf->tcp_delete(this);
                return;
            }
            if (!h->tcp.ack)
                return;
            if (seq_le(snd_una,h->tcp.ack_num) &&
                seq_le(h->tcp.ack_num,snd_nxt))
            {
                TRANSITION(TCP_ESTABLISHED);
                handle_established_segment_recvd(p);
                return;
            }
            if (h->tcp.fin)
            {
                process_fin(p);
                TRANSITION(TCP_CLOSE_WAIT);
                return;
            }

            // We're just going to close the socket; the packet could
            // get dropped.  It seems like this is unnecessary.
            tcp::post_rst(intf,p,snd_nxt);
            intf->tcp_delete(this);
        break;

        case TCP_ESTABLISHED:
            handle_established_segment_recvd(p);
        break;

        case TCP_CLOSED:
        case TCP_CLOSE_WAIT:
            intf->intf_dbg("dropping packet\n");
        break;
    }
}

void
tcp::socket::handle_listen_syn_recvd(net::rx_page* p)
{
    auto* h = p->payload_cast<ipv4_tcp_headers*>();

    // Record the remote addressing info.
    hdrs.ip.dst_ip    = h->ip.src_ip;
    hdrs.tcp.dst_port = h->tcp.src_port;

    // Sort out sequence numbers.
    rcv_nxt = h->tcp.seq_num + 1;
    irs     = h->tcp.seq_num;
    iss     = kernel::random(0,0xFFFF);
    snd_nxt = iss + 1;
    snd_una = iss;

    // Compute window sizes.  Note that window scaling does not apply to syn
    // packets.
    snd_wnd = h->tcp.window_size;
    rcv_wnd = MAX_RX_WINDOW;

    // Option default values.
    snd_wnd_shift = 0;
    rcv_wnd_shift = 0;
    snd_mss       = 1460;
    rcv_mss       = 1460;

    // Parse options.
    size_t rem         = h->tcp.offset*4 - sizeof(tcp::header);
    uint8_t* opt       = (uint8_t*)&h->tcp + sizeof(tcp::header);
    uint8_t* start     = opt;
    bool shift_present = false;
    while (rem)
    {
        switch (*opt)
        {
            case 0:     // End-of-options.
                intf->intf_dbg("%zu: opt %u - end of options\n",opt-start,*opt);
                rem = 0;
            break;

            case 1:     // No-op.
                intf->intf_dbg("%zu: opt %u - no-op\n",opt-start,*opt);
                --rem;
                ++opt;
            break;

            case 2:     // MSS value.
                intf->intf_dbg("%zu: opt %u - MSS\n",opt-start,*opt);
                if (rem < 4)
                {
                    intf->intf_dbg("truncated MSS option\n");
                    return;
                }
                if (opt[1] != 4)
                {
                    intf->intf_dbg("bad MSS length of %u\n",opt[1]);
                    return;
                }
                snd_mss = *(be_uint16_t*)(opt + 2);
                intf->intf_dbg("send MSS: %u\n",snd_mss);
                rem -= 4;
                opt += 4;
            break;

            case 3:     // Window Size Shift
                intf->intf_dbg("%zu: opt %u - window size shift\n",
                               opt-start,*opt);
                if (rem < 3)
                {
                    intf->intf_dbg("truncated WND_SHIFT option\n");
                    return;
                }
                if (opt[1] != 3)
                {
                    intf->intf_dbg("bad WND_SHIFT length of %u\n",opt[1]);
                    return;
                }
                snd_wnd_shift = opt[2];
                if (snd_wnd_shift > 14)
                {
                    intf->intf_dbg("large WND_SHIFT value of %u, using 14\n",
                                   snd_wnd_shift);
                    snd_wnd_shift = 14;
                }
                rem          -= 3;
                opt          += 3;
                shift_present = true;
            break;

            default:    // Anything else.
                if (rem < 2)
                {
                    intf->intf_dbg("option %u missing length\n",opt[0]);
                    return;
                }
                intf->intf_dbg("%zu: opt %u - other len %u\n",
                               opt-start,*opt,opt[1]);
                if (rem < opt[1])
                {
                    intf->intf_dbg("option %u truncated\n",opt[0]);
                    return;
                }
                rem -= opt[1];
                opt += opt[1];
            break;
        }
    }

    // Segment(SEQ=ISS,ACK=RCV.NXT,CTL=SYN/ACK)
    auto* hop                 = tcp::alloc_reply(intf,p);
    hop->hdrs.tcp.seq_num     = iss;
    hop->hdrs.tcp.ack_num     = rcv_nxt;
    hop->hdrs.tcp.syn         = 1;
    hop->hdrs.tcp.ack         = 1;
    hop->hdrs.tcp.window_size = rcv_wnd;

    // Add MSS option.
    opt                      = (uint8_t*)&hop->hdrs.tcp + sizeof(tcp::header);
    start                    = opt;
    opt[0]                   = 2;
    opt[1]                   = 4;
    *(be_uint16_t*)(opt + 2) = rcv_mss;
    opt                     += 4;

    // Add window scaling option if we can.
    if (shift_present)
    {
        rcv_wnd_shift            = RX_WINDOW_SHIFT;
        opt[0]                   = 3;
        opt[1]                   = 3;
        opt[2]                   = rcv_wnd_shift;
        opt                     += 3;

        opt[0]                   = 1;
        opt                     += 1;
    }

    // Padding.
    while ((opt - start) % 4)
        *opt++ = 0;

    // Increment lengths.
    size_t opt_len         = (opt - start);
    hop->hdrs.ip.total_len = hop->hdrs.ip.total_len + opt_len;
    hop->alps[0].len      += opt_len;
    hop->hdrs.tcp.offset  += opt_len/4;

    intf->post_tx_frame(hop);
    TRANSITION(TCP_SYN_RECVD);
}

void
tcp::socket::handle_established_segment_recvd(net::rx_page* p)
{
    auto* h        = p->payload_cast<ipv4_tcp_headers*>();
    size_t seg_len = h->ip.total_len - sizeof(h->ip) - 4*h->tcp.offset;
    uint8_t* seg_data = (uint8_t*)&h->tcp + 4*h->tcp.offset;

    if (!seq_check(rcv_nxt,h->tcp.seq_num,seg_len,rcv_wnd))
    {
        if (!h->tcp.rst)
            tcp::post_ack(intf,p,snd_nxt,rcv_nxt,rcv_wnd >> rcv_wnd_shift);
        return;
    }
    if (h->tcp.rst)
    {
        intf->tcp_delete(this);
        return;
    }
    if (h->tcp.syn)
    {
        tcp::post_rst(intf,p,snd_nxt);
        intf->tcp_delete(this);
        return;
    }
    // ES2:
    if (!h->tcp.ack)
        return;
    if (seq_lt(snd_una,h->tcp.ack_num) &&
        seq_le(h->tcp.ack_num,snd_nxt))
    {
        window_update(seg_len);
        snd_wnd = kernel::min(cwnd,
                              (uint32_t)h->tcp.window_size << snd_wnd_shift);
        // TODO: Remove any segments from retransmit queue that were entirely
        //       acknowledged by this ACK.
        snd_una  = h->tcp.ack_num;
        exp_boff = 1;
        // TODO: "Release REXMT timer"?????
    }
    else if (seq_ne(h->tcp.ack_num,snd_una))
        return;
    else
    {
        ++dup_ack;
        if (dup_ack > 2)
        {
            // ...ES4...
            // TODO: Send side stuff in here.
        }
        else if (dup_ack == 2)
        {
            // TODO: "Release REXMT timer"?????
            // TODO: WTF: Segment(SEQ=SEG.ACK,ACK=[?],CTL=[?])
            ss_thresh = kernel::max(2U,kernel::min(cwnd,snd_wnd/2));
            cwnd      = ss_thresh + 3;
        }
    }
    // ES3:
    if (seg_len) // TERRY ADDED THIS CHECK TO SUPPRESS DUPLICATE ACKS
    {
        if (h->tcp.seq_num == rcv_nxt)
        {
            // TODO: Consume from OO RCV Buffer here.
            rcv_nxt = h->tcp.seq_num + seg_len;
            tcp::post_ack(intf,p,snd_nxt,rcv_nxt,rcv_wnd >> rcv_wnd_shift);

            p->client_offset = seg_data - p->payload;
            p->client_len    = seg_len;
            p->flags         = NRX_FLAG_NO_DELETE;
            rq->append(p);
        }
        else
        {
            tcp::post_ack(intf,p,snd_nxt,rcv_nxt,rcv_wnd >> rcv_wnd_shift);
            // TODO: OO RCV Buffer here.
            rcv_wnd += seg_len;
        }
    }
    if (h->tcp.fin)
    {
        process_fin(p);
        TRANSITION(TCP_CLOSE_WAIT);
        return;
    }
}

void
tcp::socket::dump_socket()
{
    intf->intf_dbg("%u.%u.%u.%u:%u <-> %u.%u.%u.%u:%u state=%u snd_mss=%u "
                   "snd_window=%u snd_shift=%u rcv_mss=%u rcv_window=%u "
                   "rcv_shift=%u\n",
                   hdrs.ip.src_ip[0],
                   hdrs.ip.src_ip[1],
                   hdrs.ip.src_ip[2],
                   hdrs.ip.src_ip[3],
                   (uint16_t)hdrs.tcp.src_port,
                   hdrs.ip.dst_ip[0],
                   hdrs.ip.dst_ip[1],
                   hdrs.ip.dst_ip[2],
                   hdrs.ip.dst_ip[3],
                   (uint16_t)hdrs.tcp.dst_port,
                   state,
                   snd_mss,
                   snd_wnd,
                   snd_wnd_shift,
                   rcv_mss,
                   (uint16_t)(rcv_wnd >> rcv_wnd_shift),
                   rcv_wnd_shift);
}

void
tcp::socket::window_update(uint32_t seg_len)
{
    if (dup_ack > 0)
        cwnd = ss_thresh;
    else if (cwnd <= ss_thresh)
        cwnd *= 2;
    else
        cwnd += seg_len;
}

void
tcp::socket::process_fin(net::rx_page* p)
{
    auto* h = p->payload_cast<ipv4_tcp_headers*>();
    rcv_nxt = h->tcp.seq_num + 1;
    tcp::post_ack(intf,p,snd_nxt,rcv_nxt,rcv_wnd >> rcv_wnd_shift);
}
