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
    tx_ops_slab(sizeof(tcp::tx_op))
{
    kassert(llsize <= sizeof(llhdr));
    uint8_t llh[sizeof(llhdr)];
    memcpy(llh,llhdr,sizeof(llhdr));
    memset(llhdr,0xDD,sizeof(llhdr));
    memcpy(llhdr + sizeof(llhdr) - llsize,llh,llsize);

    retransmit_wqe.fn      = timer_delegate(handle_retransmit_expiry);
    retransmit_wqe.args[0] = (uint64_t)this;

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
    memcpy(top->llhdr,llhdr,sizeof(llhdr));

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
    top->format_rst(seq_num);
    post_op(top);
}

void
tcp::socket::post_rst_ack(uint32_t ack_num)
{
    auto* top = alloc_tx_op();
    top->format_rst_ack(ack_num);
    post_op(top);
}

void
tcp::socket::post_ack(uint32_t seq_num, uint32_t ack_num, uint16_t window_size)
{
    auto* top = alloc_tx_op();
    top->format_ack(seq_num,ack_num,window_size);
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
    auto* hop                 = alloc_tx_op();
    hop->hdrs.tcp.seq_num     = iss;
    hop->hdrs.tcp.ack_num     = rcv_nxt;
    hop->hdrs.tcp.syn         = 1;
    hop->hdrs.tcp.ack         = 1;
    hop->hdrs.tcp.window_size = MIN(rcv_wnd,0xFFFFU);

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
