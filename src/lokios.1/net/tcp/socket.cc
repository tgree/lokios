#include "socket.h"
#include "traits.h"
#include "net/interface.h"
#include "k++/random.h"

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

tcp::socket::socket(net::interface* intf, net::rx_page* p):
    intf(intf),
    state(TCP_LISTEN),
    prev_state(TCP_CLOSED),
    llsize(intf->format_ll_reply(p,&llhdr,sizeof(llhdr))),
    remote_ip(p->payload_cast<tcp::ipv4_tcp_headers*>()->ip.src_ip),
    local_port(p->payload_cast<tcp::ipv4_tcp_headers*>()->tcp.dst_port),
    remote_port(p->payload_cast<tcp::ipv4_tcp_headers*>()->tcp.src_port),
    tx_ops_slab(sizeof(tcp::tx_op))
{
    kassert(llsize <= sizeof(llhdr));
    uint8_t llh[sizeof(llhdr)];
    memcpy(llh,llhdr,sizeof(llhdr));
    memset(llhdr,0xDD,sizeof(llhdr));
    memcpy(llhdr + sizeof(llhdr) - llsize,llh,llsize);

    retransmit_wqe.fn      = timer_delegate(handle_retransmit_expiry);
    retransmit_wqe.args[0] = (uint64_t)this;

    iss           = kernel::random(0,0xFFFF);
    snd_una       = iss;
    snd_nxt       = iss + 1;
    snd_wnd       = 0;
    snd_wnd_shift = 0;
    snd_mss       = 1460;

    irs           = 0;
    rcv_nxt       = 0;
    rcv_wnd       = MAX_RX_WINDOW;
    rcv_wnd_shift = 0;
    rcv_mss       = 1460;
}

tcp::tx_op*
tcp::socket::alloc_tx_op()
{
    auto* top          = tx_ops_slab.alloc<tx_op>();
    top->flags         = NTX_FLAG_INSERT_IP_CSUM |
                         NTX_FLAG_INSERT_TCP_CSUM;
    top->nalps         = 1;
    top->alps[0].paddr = kernel::virt_to_phys(&top->hdrs.ip) - llsize;
    top->alps[0].len   = llsize + sizeof(ipv4::header) + sizeof(tcp::header);
    memcpy(top->llhdr,llhdr,sizeof(llhdr));
    top->hdrs.init(SIP{intf->ip_addr},DIP{remote_ip},
                   SPORT{local_port},DPORT{remote_port});

    return top;
}

void
tcp::socket::free_tx_op(tcp::tx_op* top)
{
    tx_ops_slab.free(top);
}

void
tcp::socket::post_op(tcp::tx_op* top)
{
    top->cb = method_delegate(send_complete);
    posted_ops.push_back(&top->tcp_link);
    intf->post_tx_frame(top);
}

void
tcp::socket::post_rst(uint32_t seq_num)
{
    auto* top = alloc_tx_op();
    top->hdrs.format(SEQ{seq_num},CTL{FRST});
    post_op(top);
}

void
tcp::socket::post_rst_ack(uint32_t ack_num)
{
    auto* top = alloc_tx_op();
    top->hdrs.format(SEQ{0},ACK{ack_num},CTL{FRST|FACK});
    post_op(top);
}

void
tcp::socket::post_ack(uint32_t seq_num, uint32_t ack_num, size_t window_size,
    uint8_t window_shift)
{
    auto* top = alloc_tx_op();
    top->hdrs.format(SEQ{seq_num},ACK{ack_num},CTL{FACK},
                     WS{window_size,window_shift});
    post_op(top);
}

void
tcp::socket::send_complete(net::tx_op* nop)
{
    auto* top = static_cast<tcp::tx_op*>(nop);
    kassert(klist_front(posted_ops,tcp_link) == top);
    posted_ops.pop_front();
    free_tx_op(top);
}

void
tcp::socket::handle_retransmit_expiry(kernel::timer_entry*)
{
    kernel::panic("unexpected retransmit timer expiry");
}

uint64_t
tcp::socket::handle_rx_ipv4_tcp_frame(net::rx_page* p)
{
    auto* h        = p->payload_cast<ipv4_tcp_headers*>();
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
                post_rst(h->tcp.ack_num);
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

        case TCP_CLOSED:
        case TCP_SYN_RECVD:
            intf->intf_dbg("dropping packet\n");
        break;
    }

    return 0;
}

uint64_t
tcp::socket::handle_listen_syn_recvd(net::rx_page* p)
{
    auto* h = p->payload_cast<ipv4_tcp_headers*>();

    // Verify the remote addressing info.
    kassert(remote_ip == h->ip.src_ip);
    kassert(remote_port == h->tcp.src_port);

    // Sort out sequence numbers.
    irs     = h->tcp.seq_num;
    rcv_nxt = irs + 1;

    // Compute window sizes.  Note that window scaling does not apply to syn
    // packets.
    snd_wnd = h->tcp.window_size;

    // Parse options.
    size_t rem         = h->tcp.offset*4 - sizeof(tcp::header);
    uint8_t* opt       = h->tcp.options;
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
                    return 0;
                }
                if (opt[1] != 4)
                {
                    intf->intf_dbg("bad MSS length of %u\n",opt[1]);
                    return 0;
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
                    return 0;
                }
                if (opt[1] != 3)
                {
                    intf->intf_dbg("bad WND_SHIFT length of %u\n",opt[1]);
                    return 0;
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
                    return 0;
                }
                intf->intf_dbg("%zu: opt %u - other len %u\n",
                               opt-start,*opt,opt[1]);
                if (rem < opt[1])
                {
                    intf->intf_dbg("option %u truncated\n",opt[0]);
                    return 0;
                }
                rem -= opt[1];
                opt += opt[1];
            break;
        }
    }

    // Segment(SEQ=ISS,ACK=RCV.NXT,CTL=SYN/ACK)
    auto* hop = alloc_tx_op();
    hop->hdrs.format(SEQ{iss},ACK{rcv_nxt},CTL{FSYN|FACK},WS{rcv_wnd,0});

    // Add MSS option.
    opt                      = hop->hdrs.tcp.options;
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

    // Post it.
    post_op(hop);
    TRANSITION(TCP_SYN_RECVD);
    return 0;
}

void
tcp::socket::dump_socket()
{
    intf->intf_dbg("%u.%u.%u.%u:%u <-> %u.%u.%u.%u:%u state=%u snd_mss=%u "
                   "snd_window=%u snd_shift=%u rcv_mss=%u rcv_window=%u "
                   "rcv_shift=%u\n",
                   intf->ip_addr[0],
                   intf->ip_addr[1],
                   intf->ip_addr[2],
                   intf->ip_addr[3],
                   local_port,
                   remote_ip[0],
                   remote_ip[1],
                   remote_ip[2],
                   remote_ip[3],
                   remote_port,
                   state,
                   snd_mss,
                   snd_wnd,
                   snd_wnd_shift,
                   rcv_mss,
                   (uint16_t)(rcv_wnd >> rcv_wnd_shift),
                   rcv_wnd_shift);
}
