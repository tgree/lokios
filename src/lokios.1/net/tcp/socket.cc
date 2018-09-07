#include "socket.h"
#include "traits.h"
#include "net/interface.h"
#include "k++/random.h"
#include "kern/cpu.h"
#include "kern/console.h"

#define RETRANSMIT_TIMEOUT_MS   1000
#define TIME_WAIT_TIMEOUT_SEC   60*4

#define DEBUG_TRANSITIONS 0

#if DEBUG_TRANSITIONS
#define TRANSITION(s) \
    do                                                                   \
    {                                                                    \
        dbg("%s:%u: %u -> " #s "\n",__FILE__,__LINE__,state); \
        state = (s);                                                     \
    } while(0)
#else
#define TRANSITION(s) state = (s)
#endif

using kernel::_kassert;

struct fin_recvd_exception {uint64_t flags;};
struct header_invalid_exception {const char* msg;};
struct ack_unacceptable_exception {};
struct socket_reset_exception {};

static constexpr uint8_t
compute_wnd_shift(uint32_t wnd)
{
    return CLAMP(15U,kernel::ulog2(wnd),29U) - 15U;
}
KASSERT(compute_wnd_shift(0x00008000) == 0);
KASSERT(compute_wnd_shift(0x0000FFFF) == 0);
KASSERT(compute_wnd_shift(0x00010000) == 1);
KASSERT(compute_wnd_shift(0x0001FFFF) == 1);
KASSERT(compute_wnd_shift(0x00020000) == 2);
KASSERT(compute_wnd_shift(0x0003FFFF) == 2);
KASSERT(compute_wnd_shift(0x00040000) == 3);
KASSERT(compute_wnd_shift(0x1FFFFFFF) == 13);
KASSERT(compute_wnd_shift(0x20000000) == 14);
KASSERT(compute_wnd_shift(0x3FFFFFFF) == 14);
KASSERT(compute_wnd_shift(0x40000000) == 14);
KASSERT(compute_wnd_shift(0x80000000) == 14);
KASSERT(compute_wnd_shift(0xFFFFFFFF) == 14);

uint32_t
tcp::send_op::mark_acked(uint32_t ack_len)
{
    kassert(ack_len != 0);

    if (flags & SEND_OP_FLAG_SYN)
    {
        flags &= ~SEND_OP_FLAG_SYN;
        --ack_len;
    }

    auto* alp = &alps[unacked_alp_index];
    while (ack_len && unacked_alp_index != nalps)
    {
        auto len = MIN(ack_len,alp->len - unacked_alp_offset);

        unacked_alp_offset += len;
        ack_len            -= len;
        if (unacked_alp_offset == alp->len)
        {
            ++alp;
            ++unacked_alp_index;
            unacked_alp_offset = 0;
        }
    }

    if (ack_len && (flags & SEND_OP_FLAG_FIN))
    {
        flags &= ~SEND_OP_FLAG_FIN;
        --ack_len;
    }

    return ack_len; 
}

bool
tcp::send_op::is_fully_acked() const
{
    if (flags & (SEND_OP_FLAG_SYN | SEND_OP_FLAG_FIN))
        return false;
    return unacked_alp_index == nalps;
}

tcp::socket::socket(net::interface* intf, net::rx_page* p,
    parsed_options rx_opts, uint32_t rcv_wnd):
        intf(intf),
        state(TCP_CLOSED),
        llsize(intf->format_ll_reply(p,&llhdr,sizeof(llhdr))),
        remote_ip(p->payload_cast<tcp::ipv4_tcp_headers*>()->ip.src_ip),
        local_port(p->payload_cast<tcp::ipv4_tcp_headers*>()->tcp.dst_port),
        remote_port(p->payload_cast<tcp::ipv4_tcp_headers*>()->tcp.src_port),
        observer(NULL),
        rx_opts(rx_opts),
        rcv_wnd(rcv_wnd)
{
    auto* h = p->payload_cast<ipv4_tcp_headers*>();
    kassert(!h->tcp.rst);
    kassert(!h->tcp.ack);
    kassert(h->tcp.syn);
    kassert(llsize <= sizeof(llhdr));
    uint8_t llh[sizeof(llhdr)];
    memcpy(llh,llhdr,sizeof(llhdr));
    memset(llhdr,0xDD,sizeof(llhdr));
    memcpy(llhdr + sizeof(llhdr) - llsize,llh,llsize);

    snd_wnd = h->tcp.window_size;
    snd_mss = MIN(MAX_SAFE_IP_SIZE,intf->tx_mtu) - sizeof(ipv4_tcp_headers);

    rcv_nxt       = h->tcp.seq_num + 1;
    rcv_wnd_shift = 0;
    rcv_mss       = intf->rx_mtu - sizeof(ipv4_tcp_headers);

    process_options(rx_opts);

    send(0,NULL,method_delegate(syn_send_op_cb),
         SEND_OP_FLAG_SYN | SEND_OP_FLAG_SET_ACK |
         ((rx_opts.flags & OPTION_SND_WND_SHIFT_PRESENT)
            ? SEND_OP_FLAG_SET_SCALE : 0));
    TRANSITION(TCP_SYN_RECVD);
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
        rcv_wnd(rcv_wnd)
{
    kassert(llsize <= sizeof(llhdr));
    memset(llhdr,0xDD,sizeof(llhdr));
    memcpy(llhdr + sizeof(llhdr) - llsize,llh,llsize);

    snd_wnd = 1;
    snd_mss = MIN(MAX_SAFE_IP_SIZE,intf->tx_mtu) - sizeof(ipv4_tcp_headers);

    rcv_nxt       = 0;
    rcv_wnd_shift = compute_wnd_shift(rcv_wnd);
    rcv_mss       = intf->rx_mtu - sizeof(ipv4_tcp_headers);

    send(0,NULL,method_delegate(syn_send_op_cb),
         SEND_OP_FLAG_SYN | SEND_OP_FLAG_SET_SCALE);
    TRANSITION(TCP_SYN_SENT);
}

tcp::socket::~socket()
{
    // If the client has left any rx_pages on the receive queue, then at this
    // point they don't care about them.
    while (!rx_pages.empty())
    {
        auto* p = klist_front(rx_pages,link);
        rx_pages.pop_front();
        intf->free_rx_page(p);
    }
}

tcp::tx_op*
tcp::socket::alloc_tx_op(tcp::send_op* sop)
{
    auto* top          = tx_ops_slab.alloc<tx_op>();
    top->sop           = sop;
    top->flags         = NTX_FLAG_INSERT_IP_CSUM |
                         NTX_FLAG_INSERT_TCP_CSUM;
    top->nalps         = 1;
    top->alps[0].paddr = kernel::virt_to_phys(&top->hdrs.ip) - llsize;
    top->alps[0].len   = llsize + sizeof(ipv4::header) + sizeof(tcp::header);
    memcpy(top->llhdr,llhdr,sizeof(llhdr));
    top->hdrs.init(SIP{intf->ip_addr},DIP{remote_ip},
                   SPORT{local_port},DPORT{remote_port});

    if (sop)
        sop->posted_ops.push_back(&top->sop_link);
    return top;
}

void
tcp::socket::free_tx_op(tcp::tx_op* top)
{
    auto* sop = top->sop;
    if (sop)
    {
        kassert(!sop->posted_ops.empty());
        kassert(klist_front(sop->posted_ops,sop_link) == top);
        sop->posted_ops.pop_front();
        if (sop->posted_ops.empty() && sop->is_fully_acked())
            process_ack_queue();
    }
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
tcp::socket::post_retransmittable_op(tcp::tx_op* top)
{
    top->cb = method_delegate(send_complete_arm_retransmit);
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
tcp::socket::post_ack()
{
    auto* top = alloc_tx_op();
    top->hdrs.format(SEQ{snd_nxt},ACK{rcv_nxt},CTL{FACK},
                     WS{rcv_wnd,rcv_wnd_shift});
    post_op(top);
}

void
tcp::socket::post_update_ack()
{
    // If this is the same as the last update we sent, skip this update.
    if (last_ack_wnd_size == rcv_wnd && last_ack_ack_num == rcv_nxt)
        return;

    // If something we have queued up is about to get sent, skip this update.
    if (!unsent_send_ops.empty() && !is_send_window_full())
        return;

    last_ack_wnd_size = rcv_wnd;
    last_ack_ack_num  = rcv_nxt;
    post_ack();
}

void
tcp::socket::send_complete(net::tx_op* nop)
{
    auto* top = static_cast<tcp::tx_op*>(nop);
    kassert(klist_front(posted_ops,tcp_link) == top);
    posted_ops.pop_front();
    free_tx_op(top);

    if (state == TCP_DRAIN_NIC_OPS && posted_ops.empty())
        socket_closed();
}

void
tcp::socket::send_complete_arm_retransmit(net::tx_op* nop)
{
    auto* top = static_cast<tcp::tx_op*>(nop);
    if (!retransmit_wqe.is_armed() && !top->sop->is_fully_acked())
        kernel::cpu::schedule_timer_ms(&retransmit_wqe,RETRANSMIT_TIMEOUT_MS);
    send_complete(nop);
}

tcp::send_op*
tcp::socket::send(size_t nalps, const kernel::dma_alp* alps,
    kernel::delegate<void(send_op*)> cb, uint64_t flags)
{
    auto* sop               = send_ops_slab.alloc<tcp::send_op>();
    sop->cb                 = cb;
    sop->flags              = flags;
    sop->nalps              = nalps;
    sop->unacked_alp_index  = 0;
    sop->unsent_alp_index   = 0;
    sop->unacked_alp_offset = 0;
    sop->unsent_alp_offset  = 0;
    sop->alps               = alps;

    switch (state)
    {
        case TCP_CLOSED:
            kassert(flags & SEND_OP_FLAG_SYN);
        case TCP_ESTABLISHED:
        case TCP_CLOSE_WAIT:
            unsent_send_ops.push_back(&sop->link);
            process_send_queue();
        break;

        case TCP_DRAIN_NIC_OPS:
        case TCP_SYN_SENT:
        case TCP_SYN_SENT_ACKED_WAIT_SYN:
        case TCP_SYN_SENT_SYN_RECVD_WAIT_ACK:
        case TCP_SYN_RECVD:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
        case TCP_CLOSING:
        case TCP_TIME_WAIT:
        case TCP_LAST_ACK:
            kernel::panic("Bad state for send!");
        break;
    }

    return sop;
}

tcp::tx_op*
tcp::socket::make_one_packet(tcp::send_op* sop)
{
    auto avail_len = MIN(snd_una + snd_wnd - snd_nxt,snd_mss);
    kassert(avail_len > 0);

    // Format the header based on if this is the SYN packet or not.
    tcp::tx_op* top = alloc_tx_op(sop);
    top->hdrs.format(SEQ{snd_nxt});
    if (!(sop->flags & SEND_OP_FLAG_SYN) ||
        sop->unsent_alp_index > 0 ||
        sop->unsent_alp_offset > 0)
    {
        top->hdrs.format(ACK{rcv_nxt},CTL{FACK},WS{rcv_wnd,rcv_wnd_shift});
    }
    else
    {
        top->hdrs.format(CTL{FSYN},WS{rcv_wnd,0});
        if (sop->flags & SEND_OP_FLAG_SET_ACK)
            top->hdrs.format(ACK{rcv_nxt},CTL{FSYN|FACK});

        parsed_options opts;
        opts.flags         = OPTION_SND_MSS_PRESENT;
        opts.snd_mss       = rcv_mss;
        opts.snd_wnd_shift = rcv_wnd_shift;
        if (sop->flags & SEND_OP_FLAG_SET_SCALE)
            opts.flags |= OPTION_SND_WND_SHIFT_PRESENT;

        size_t opt_len = top->hdrs.tcp.append_options(opts);
        top->hdrs.ip.total_len += opt_len;
        top->alps[0].len       += opt_len;

        // Reduce avail_len if necessary so that payload plus options won't
        // exceed snd_mss.
        if (avail_len + opt_len > snd_mss)
            avail_len = snd_mss - opt_len;

        // Also reduce avail_len for the SYN sequence number.
        --avail_len;
    }

    // Drop some alps in if we have payload remaining to process.
    kassert(top->nalps == 1);
    const kernel::dma_alp* salp = &sop->alps[sop->unsent_alp_index];
    kernel::dma_alp* dalp       = &top->alps[1];
    while (avail_len &&
           sop->unsent_alp_index < sop->nalps &&
           top->nalps < NELEMS(top->alps))
    {
        dalp->paddr = salp->paddr + sop->unsent_alp_offset;
        dalp->len   = MIN(salp->len - sop->unsent_alp_offset,avail_len);

        sop->unsent_alp_offset += dalp->len;
        if (sop->unsent_alp_offset == salp->len)
        {
            sop->unsent_alp_offset = 0;
            ++sop->unsent_alp_index;
            ++salp;
        }

        if (dalp->len)
        {
            top->hdrs.ip.total_len += dalp->len;
            avail_len              -= dalp->len;
            ++top->nalps;
            ++dalp;
        }
    }

    // Do PSH and FIN processing if we're at the end of the send op.
    if (sop->unsent_alp_index == sop->nalps)
    {
        if (sop->nalps)
            top->hdrs.tcp.psh = 1;
        if (sop->flags & SEND_OP_FLAG_FIN)
        {
            if (avail_len)
            {
                top->hdrs.tcp.fin = 1;
                unsent_send_ops.pop_front();
                sent_send_ops.push_back(&sop->link);
            }
        }
        else
        {
            unsent_send_ops.pop_front();
            sent_send_ops.push_back(&sop->link);
        }
    }

    return top;
}

void
tcp::socket::process_send_queue()
{
    if (!seq_range{snd_una,snd_wnd}.seq_in_range(snd_nxt))
    {
        dbg("snd window empty snd_nxt %u snd_una %u snd_wnd %u\n",
            snd_nxt,snd_una,snd_wnd);
        return;
    }

    while (!unsent_send_ops.empty() && snd_nxt != snd_una + snd_wnd)
    {
        tcp::send_op* sop = klist_front(unsent_send_ops,link);
        tcp::tx_op* top   = make_one_packet(sop);
        last_ack_wnd_size = top->hdrs.tcp.window_size;
        last_ack_ack_num  = top->hdrs.tcp.ack_num;
        snd_nxt          += top->hdrs.segment_len();
        post_retransmittable_op(top);
    }
}

void
tcp::socket::process_ack(uint32_t ack_num, uint32_t lower_bound)
{
    // snd_una is the first unacknowledged byte and corresponds to the head of
    // the sent_send_ops queue.
    //
    // ack_num is the first unseen sequence number by the remote guy.  So he
    // is acking [snd_una,ack_num).  I.e. sequence number ack_num is NOT being
    // acknowledged yet.
    seq_range valid_ack_seqs = seq_bound(lower_bound,snd_nxt);
    if (!valid_ack_seqs.seq_in_range(ack_num))
        throw ack_unacceptable_exception();

    uint32_t ack_len = ack_num - snd_una;
    if (ack_len && retransmit_wqe.is_armed())
    {
        kernel::cpu::cancel_timer(&retransmit_wqe);
        kernel::cpu::schedule_timer_ms(&retransmit_wqe,RETRANSMIT_TIMEOUT_MS);
    }

    while (ack_len && !sent_send_ops.empty())
    {
        auto* sop = klist_front(sent_send_ops,link);
        ack_len   = sop->mark_acked(ack_len);

        if (sop->is_fully_acked())
        {
            sent_send_ops.pop_front();
            acked_send_ops.push_back(&sop->link);
        }
    }
    if (ack_len && !unsent_send_ops.empty())
    {
        auto* sop = klist_front(unsent_send_ops,link);
        ack_len   = sop->mark_acked(ack_len);
        kassert(sop->unacked_alp_index < sop->nalps-1 ||
                sop->unacked_alp_offset < sop->alps[sop->nalps-1].len);
    }
    kassert(ack_len == 0);

    snd_una = ack_num;

    if (snd_una == snd_nxt && retransmit_wqe.is_armed())
        kernel::cpu::cancel_timer(&retransmit_wqe);

    process_ack_queue();
}

void
tcp::socket::process_ack_queue()
{
    while (!acked_send_ops.empty())
    {
        auto* sop = klist_front(acked_send_ops,link);
        if (!sop->posted_ops.empty())
            break;

        acked_send_ops.pop_front();
        sop->cb(sop);
        send_ops_slab.free(sop);
    }
}

void
tcp::socket::rx_append(net::rx_page* p)
{
    rx_pages.push_back(&p->link);
    rx_avail_bytes += p->client_len;
    rcv_wnd        -= p->client_len;
    if (state >= TCP_ESTABLISHED)
        observer->socket_readable(this);
}

void
tcp::socket::read(void* _dst, uint32_t rem)
{
    kassert(rem <= rx_avail_bytes);
    rx_avail_bytes -= rem;
    rcv_wnd        += rem;

    char* dst = (char*)_dst;
    while (rem)
    {
        net::rx_page* p = klist_front(rx_pages,link);
        uint32_t len    = MIN(rem,p->client_len);
        memcpy(dst,p->payload + p->client_offset,len);
        p->client_offset += len;
        p->client_len    -= len;
        dst              += len;
        rem              -= len;

        if (!p->client_len)
        {
            rx_pages.pop_front();
            intf->free_rx_page(p);
        }
    }

    post_update_ack();
}

void
tcp::socket::skip(uint32_t rem)
{
    kassert(rem <= rx_avail_bytes);
    rx_avail_bytes -= rem;
    rcv_wnd        += rem;

    while (rem)
    {
        net::rx_page* p   = klist_front(rx_pages,link);
        uint32_t len      = MIN(rem,p->client_len);
        p->client_offset += len;
        p->client_len    -= len;
        rem              -= len;

        if (!p->client_len)
        {
            rx_pages.pop_front();
            intf->free_rx_page(p);
        }
    }

    post_update_ack();
}

void
tcp::socket::skip_page()
{
    net::rx_page* p = klist_front(rx_pages,link);
    rx_avail_bytes -= p->client_len;
    rcv_wnd        += p->client_len;
    rx_pages.pop_front();
    intf->free_rx_page(p);
    post_update_ack();
}

void
tcp::socket::syn_send_op_cb(tcp::send_op* sop)
{
    switch (state)
    {
        case TCP_SYN_SENT:
            TRANSITION(TCP_SYN_SENT_ACKED_WAIT_SYN);
        break;

        case TCP_SYN_RECVD:
        case TCP_SYN_SENT_SYN_RECVD_WAIT_ACK:
            TRANSITION(TCP_ESTABLISHED);
            observer->socket_established(this);
            if (rx_avail_bytes)
                observer->socket_readable(this);
        break;

        case TCP_DRAIN_NIC_OPS:
        break;

        case TCP_CLOSED:
        case TCP_SYN_SENT_ACKED_WAIT_SYN:
        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
        case TCP_CLOSING:
        case TCP_TIME_WAIT:
        case TCP_CLOSE_WAIT:
        case TCP_LAST_ACK:
            kernel::panic("Bad state for syn_send_op_cb!");
        break;
    }
}

void
tcp::socket::fin_send_op_cb(tcp::send_op* sop)
{
    switch (state)
    {
        case TCP_FIN_WAIT_1:
            TRANSITION(TCP_FIN_WAIT_2);
        break;

        case TCP_CLOSING:
            TRANSITION(TCP_TIME_WAIT);
            kernel::cpu::schedule_timer_sec(&time_wait_wqe,
                                            TIME_WAIT_TIMEOUT_SEC);
        break;

        case TCP_LAST_ACK:
            // Our FIN has been ACK'd and all send_ops prior to it have
            // completed.
            socket_drain();
        break;

        case TCP_DRAIN_NIC_OPS:
        break;

        case TCP_CLOSED:
        case TCP_SYN_SENT:
        case TCP_SYN_SENT_ACKED_WAIT_SYN:
        case TCP_SYN_SENT_SYN_RECVD_WAIT_ACK:
        case TCP_SYN_RECVD:
        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT_2:
        case TCP_TIME_WAIT:
        case TCP_CLOSE_WAIT:
            kernel::panic("Bad state for fin_send_op_cb!");
        break;
    }
}

void
tcp::socket::handle_retransmit_expiry(kernel::tqe*)
{
    if (!unsent_send_ops.empty())
    {
        auto* sop = klist_front(unsent_send_ops,link);
        unsent_send_ops.pop_front();
        sent_send_ops.push_back(&sop->link);
    }
    for (auto& sop : klist_elems(sent_send_ops,link))
    {
        sop.unsent_alp_index  = sop.unacked_alp_index;
        sop.unsent_alp_offset = sop.unacked_alp_offset;
    }
    sent_send_ops.append(unsent_send_ops);
    unsent_send_ops.append(sent_send_ops);
    snd_nxt = snd_una;
    process_send_queue();
}

void
tcp::socket::handle_time_wait_expiry(kernel::tqe* wqe)
{
    kassert(state == TCP_TIME_WAIT);
    socket_drain();
}

void
tcp::socket::handle_socket_closed_wqe(kernel::wqe* wqe)
{
    observer->socket_closed(this);
}

uint64_t
tcp::socket::handle_rx_ipv4_tcp_frame(net::rx_page* p) try
{
    auto* h = p->payload_cast<ipv4_tcp_headers*>();
    switch (state)
    {
        case TCP_SYN_SENT:
        case TCP_SYN_SENT_ACKED_WAIT_SYN:
            if (h->tcp.ack)
            {
                process_ack(h->tcp.ack_num,iss+1);
                if (h->tcp.rst)
                    throw socket_reset_exception();
            }
            else if (h->tcp.rst)
                throw header_invalid_exception{"rst bit set"};
            if (h->tcp.syn)
            {
                rx_opts = h->tcp.parse_options();
                process_options(rx_opts);

                snd_wnd = h->tcp.window_size;
                rcv_nxt = h->tcp.seq_num + 1;

                post_update_ack();

                if (state == TCP_SYN_SENT_ACKED_WAIT_SYN)
                {
                    TRANSITION(TCP_ESTABLISHED);
                    observer->socket_established(this);
                    if (rx_avail_bytes)
                        observer->socket_readable(this);
                    break;
                }
                else
                    TRANSITION(TCP_SYN_SENT_SYN_RECVD_WAIT_ACK);
            }
        break;

        case TCP_SYN_RECVD:
        case TCP_ESTABLISHED:
        case TCP_SYN_SENT_SYN_RECVD_WAIT_ACK:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
            process_header_synchronized(h);
            return process_payload_synchronized(p);
        break;

        case TCP_CLOSING:
        case TCP_CLOSE_WAIT:
        case TCP_TIME_WAIT:
        case TCP_LAST_ACK:
            process_header_synchronized(h);
        break;

        case TCP_CLOSED:
        case TCP_DRAIN_NIC_OPS:
            kernel::panic("rx packet when we should be unlinked");
        break;
    }

    return 0;
}
catch (fin_recvd_exception& e)
{
    switch (state)
    {
        case TCP_SYN_RECVD:
        case TCP_ESTABLISHED:
        case TCP_SYN_SENT_SYN_RECVD_WAIT_ACK:
            TRANSITION(TCP_CLOSE_WAIT);
        break;

        case TCP_FIN_WAIT_1:
            TRANSITION(TCP_CLOSING);
        break;

        case TCP_FIN_WAIT_2:
            TRANSITION(TCP_TIME_WAIT);
            kernel::cpu::schedule_timer_sec(&time_wait_wqe,
                                            TIME_WAIT_TIMEOUT_SEC);
        break;

        case TCP_CLOSED:
        case TCP_DRAIN_NIC_OPS:
        case TCP_SYN_SENT:
        case TCP_SYN_SENT_ACKED_WAIT_SYN:
        case TCP_CLOSING:
        case TCP_TIME_WAIT:
        case TCP_CLOSE_WAIT:
        case TCP_LAST_ACK:
            kernel::panic("fin-recvd thrown from impossible state");
        break;
    }
    observer->socket_recv_closed(this);
    return e.flags;
}
catch (socket_reset_exception)
{
    switch (state)
    {
        case TCP_SYN_SENT_ACKED_WAIT_SYN:
        case TCP_SYN_SENT_SYN_RECVD_WAIT_ACK:
        case TCP_FIN_WAIT_2:
            kassert(!retransmit_wqe.is_armed());
            kassert(!time_wait_wqe.is_armed());
        break;

        case TCP_SYN_SENT:
        case TCP_SYN_RECVD:
        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT_1:
        case TCP_CLOSING:
        case TCP_CLOSE_WAIT:
        case TCP_LAST_ACK:
            if (retransmit_wqe.is_armed())
                kernel::cpu::cancel_timer(&retransmit_wqe);
            kassert(!time_wait_wqe.is_armed());
        break;

        case TCP_TIME_WAIT:
            kassert(!retransmit_wqe.is_armed());
            kernel::cpu::cancel_timer(&time_wait_wqe);
        break;

        case TCP_DRAIN_NIC_OPS:
        case TCP_CLOSED:
            kernel::panic("rst-received thrown from impossible state");
        break;
    }

    observer->socket_reset(this);
    socket_drain();
    return 0;
}
catch (const header_invalid_exception& e)
{
    return 0;
}
catch (ack_unacceptable_exception)
{
    auto* h = p->payload_cast<ipv4_tcp_headers*>();
    switch (state)
    {
        case TCP_SYN_SENT:
        case TCP_SYN_SENT_ACKED_WAIT_SYN:
        case TCP_SYN_RECVD:
            if (!h->tcp.rst)
                post_rst(h->tcp.ack_num);
        break;

        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
        case TCP_CLOSE_WAIT:
        case TCP_CLOSING:
        case TCP_LAST_ACK:
        case TCP_TIME_WAIT:
            // If the ACK is duplicate (SEG.ACK < SND_UNA) then it can be
            // ignored.  Otherwise, if it's from the future (SEG.ACK > SND.NXT)
            // we have to send an ACK and drop it.  It's not clear how to
            // define the future - we'll check a 1G window from SND.NXT.
            if (seq_range{snd_nxt,0x40000000}.seq_in_range(h->tcp.ack_num))
                post_ack();
        break;

        case TCP_SYN_SENT_SYN_RECVD_WAIT_ACK:
        break;

        case TCP_CLOSED:
        case TCP_DRAIN_NIC_OPS:
            kernel::panic("ack-unacceptable thrown from impossible state");
        break;
    }
    return 0;
}
catch (option_parse_exception& e)
{
    dbg("option parse error: %s (%lu)\n",e.msg,e.val);
    return 0;
}

void
tcp::socket::close_send()
{
    switch (state)
    {
        case TCP_SYN_SENT:
        case TCP_SYN_SENT_ACKED_WAIT_SYN:
        case TCP_SYN_SENT_SYN_RECVD_WAIT_ACK:
            socket_drain();
        break;

        case TCP_SYN_RECVD:
        case TCP_ESTABLISHED:
            send(0,NULL,method_delegate(fin_send_op_cb),SEND_OP_FLAG_FIN);
            TRANSITION(TCP_FIN_WAIT_1);
        break;

        case TCP_CLOSE_WAIT:
            send(0,NULL,method_delegate(fin_send_op_cb),SEND_OP_FLAG_FIN);
            TRANSITION(TCP_LAST_ACK);
        break;

        case TCP_CLOSED:
        case TCP_DRAIN_NIC_OPS:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
        case TCP_CLOSING:
        case TCP_TIME_WAIT:
        case TCP_LAST_ACK:
            kernel::panic("close_send() in unexpected state!");
        break;
    }
}

void
tcp::socket::socket_drain()
{
    TRANSITION(TCP_DRAIN_NIC_OPS);
    intf->tcp_unlink(this);

    if (posted_ops.empty())
        socket_closed();
}

void
tcp::socket::socket_closed()
{
    kassert(posted_ops.empty());
    kassert(acked_send_ops.empty());

    kernel::kdlist<tcp::send_op> ops_list;
    ops_list.append(sent_send_ops);
    ops_list.append(unsent_send_ops);
    while (!ops_list.empty())
    {
        auto* sop = klist_front(ops_list,link);
        ops_list.pop_front();
        kassert(sop->posted_ops.empty());
        sop->flags |= SEND_OP_FLAG_UNACKED;
        sop->cb(sop);
        send_ops_slab.free(sop);
    }

    TRANSITION(TCP_CLOSED);
    kernel::cpu::schedule_local_work(&socket_closed_wqe);
}

void
tcp::socket::process_header_synchronized(const ipv4_tcp_headers* h)
{
    // We're in a state where we've received the initial SYN so we need to
    // check sequence numbers, various flags and handle ACKs to advance the
    // send queue.
    if (!seq_check(rcv_nxt,h->tcp.seq_num,h->segment_len(),rcv_wnd))
    {
        if (!h->tcp.rst)
            post_ack();
        throw header_invalid_exception{"seq check failed"};
    }
    if (h->tcp.rst)
        throw socket_reset_exception();
    if (h->tcp.syn)
    {
        post_rst(snd_nxt);
        throw socket_reset_exception();
    }
    if (!h->tcp.ack)
        throw header_invalid_exception{"ack not set"};

    process_ack(h->tcp.ack_num,snd_una);
    snd_wnd = h->tcp.window_size << snd_wnd_shift;
    process_send_queue();
}

uint64_t
tcp::socket::process_payload_synchronized(net::rx_page* p)
{
    // We're in a state where we've received SYN but haven't received a FIN
    // yet.  So we should handle payload normally, appending it to the RX queue
    // and notifying the client.
    auto* h            = p->payload_cast<ipv4_tcp_headers*>();
    bool fin           = h->tcp.fin;
    seq_range rx_range = {h->tcp.seq_num,h->segment_len()};
    seq_range new_seqs = seq_overlap(rx_range,{rcv_nxt,rcv_wnd});

    // TODO: This check identifies out-of-order packets.  Instead of just
    //       dropping the packet, we should queue it up for later
    //       processing.
    if (new_seqs.first != rcv_nxt)
        return 0;

    uint64_t flags = 0;
    if (new_seqs.len)
    {
        rcv_nxt         += new_seqs.len;
        uint32_t skip    = new_seqs.first - h->tcp.seq_num;
        p->client_len    = h->payload_len() - skip;
        if (p->client_len)
        {
            p->client_offset = (uint8_t*)h->get_payload() - p->payload + skip;
            rx_append(p);
            flags = NRX_FLAG_NO_DELETE;
        }
        post_update_ack();
    }

    if (fin)
        throw fin_recvd_exception{flags};

    return flags;
}

void
tcp::socket::process_options(parsed_options opts)
{
    if (opts.flags & OPTION_SND_MSS_PRESENT)
        snd_mss = MIN(opts.snd_mss,intf->tx_mtu - sizeof(ipv4_tcp_headers));
    if (opts.flags & OPTION_SND_WND_SHIFT_PRESENT)
    {
        snd_wnd_shift = opts.snd_wnd_shift;
        rcv_wnd_shift = compute_wnd_shift(rcv_wnd);
    }
    else
    {
        snd_wnd_shift = 0;
        rcv_wnd_shift = 0;
    }
}

void
tcp::socket::dump_socket()
{
    dbg("state=%u snd_mss=%u snd_window=%u snd_shift=%u rcv_mss=%u "
        "rcv_window=%u rcv_shift=%u\n",
        state,snd_mss,snd_wnd,snd_wnd_shift,rcv_mss,
        (uint16_t)(rcv_wnd >> rcv_wnd_shift),rcv_wnd_shift);
}

#define STATE_CASE(s) case TCP_ ## s: return #s
const char*
tcp::socket::get_state_name() const
{
    switch (state)
    {
        STATE_CASE(CLOSED);
        STATE_CASE(DRAIN_NIC_OPS);
        STATE_CASE(SYN_SENT);
        STATE_CASE(SYN_SENT_ACKED_WAIT_SYN);
        STATE_CASE(SYN_SENT_SYN_RECVD_WAIT_ACK);
        STATE_CASE(SYN_RECVD);
        STATE_CASE(ESTABLISHED);
        STATE_CASE(FIN_WAIT_1);
        STATE_CASE(FIN_WAIT_2);
        STATE_CASE(CLOSING);
        STATE_CASE(TIME_WAIT);
        STATE_CASE(CLOSE_WAIT);
        STATE_CASE(LAST_ACK);
    }
    return "UNKNOWN";
}

void
tcp::socket::vdbg(const char* fmt, va_list ap)
{
    kernel::console::p2printf(fmt,ap,"net%zu:%u.%u.%u.%u:%u:%u.%u.%u.%u:%u: ",
                              intf->id,
                              intf->ip_addr[0],
                              intf->ip_addr[1],
                              intf->ip_addr[2],
                              intf->ip_addr[3],
                              local_port,
                              remote_ip[0],
                              remote_ip[1],
                              remote_ip[2],
                              remote_ip[3],
                              remote_port);
}
