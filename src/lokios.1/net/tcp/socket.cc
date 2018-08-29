#include "socket.h"
#include "traits.h"
#include "net/interface.h"
#include "k++/random.h"
#include "kernel/cpu.h"

#define RETRANSMIT_TIMEOUT_MS   1000
#define TIME_WAIT_TIMEOUT_SEC   60*4

#define DEBUG_TRANSITIONS 0

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

struct fin_recvd_exception {uint64_t flags;};
struct header_invalid_exception {};
struct ack_unacceptable_exception {};
struct socket_reset_exception {};

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
    parsed_options rx_opts):
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
        rx_opts(rx_opts)
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

    retransmit_wqe.fn      = timer_delegate(handle_retransmit_expiry);
    retransmit_wqe.args[0] = (uint64_t)this;
    time_wait_wqe.fn       = timer_delegate(handle_time_wait_expiry);
    time_wait_wqe.args[0]  = (uint64_t)this;

    iss           = kernel::random(0,0xFFFF);
    snd_una       = iss;
    snd_nxt       = iss;
    snd_wnd       = h->tcp.window_size;
    snd_wnd_shift = 0;
    snd_mss       = MIN(MAX_SAFE_IP_SIZE,intf->tx_mtu) -
                    sizeof(ipv4_tcp_headers);

    rcv_nxt       = h->tcp.seq_num + 1;
    rcv_wnd       = MAX_RX_WINDOW;
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
    size_t llsize, socket_observer* observer):
        intf(intf),
        state(TCP_CLOSED),
        llsize(llsize),
        remote_ip(remote_ip),
        local_port(local_port),
        remote_port(remote_port),
        observer(observer),
        tx_ops_slab(sizeof(tcp::tx_op)),
        send_ops_slab(sizeof(tcp::send_op)),
        rx_avail_bytes(0)
{
    kassert(llsize <= sizeof(llhdr));
    memset(llhdr,0xDD,sizeof(llhdr));
    memcpy(llhdr + sizeof(llhdr) - llsize,llh,llsize);

    retransmit_wqe.fn      = timer_delegate(handle_retransmit_expiry);
    retransmit_wqe.args[0] = (uint64_t)this;
    time_wait_wqe.fn       = timer_delegate(handle_time_wait_expiry);
    time_wait_wqe.args[0]  = (uint64_t)this;

    iss           = kernel::random(0,0xFFFF);
    snd_una       = iss;
    snd_nxt       = iss;
    snd_wnd       = 1;
    snd_wnd_shift = 0;
    snd_mss       = MIN(MAX_SAFE_IP_SIZE,intf->tx_mtu) -
                    sizeof(ipv4_tcp_headers);

    rcv_nxt       = 0;
    rcv_wnd       = MAX_RX_WINDOW;
    rcv_wnd_shift = RX_WINDOW_SHIFT;
    rcv_mss       = intf->rx_mtu - sizeof(ipv4_tcp_headers);

    send(0,NULL,method_delegate(syn_send_op_cb),
         SEND_OP_FLAG_SYN | SEND_OP_FLAG_SET_SCALE);
    TRANSITION(TCP_SYN_SENT);
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
        ++sop->refcount;
    return top;
}

void
tcp::socket::free_tx_op(tcp::tx_op* top)
{
    auto* sop = top->sop;
    if (sop)
    {
        kassert(sop->refcount != 0);
        if (!--sop->refcount && sop->is_fully_acked())
        {
            sop->link.unlink();
            sop->cb(sop);
            send_ops_slab.free(sop);
        }
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
tcp::socket::send_complete_arm_retransmit(net::tx_op* nop)
{
    auto* top = static_cast<tcp::tx_op*>(nop);
    if (!retransmit_wqe.is_armed() && !top->sop->is_fully_acked())
        kernel::cpu::schedule_timer_ms(&retransmit_wqe,RETRANSMIT_TIMEOUT_MS);
    send_complete(nop);
}

tcp::send_op*
tcp::socket::send(size_t nalps, kernel::dma_alp* alps,
    kernel::delegate<void(send_op*)> cb, uint64_t flags)
{
    auto* sop               = send_ops_slab.alloc<tcp::send_op>();
    sop->cb                 = cb;
    sop->flags              = flags;
    sop->refcount           = 0;
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

        default:
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
    kernel::dma_alp* salp = &sop->alps[sop->unsent_alp_index];
    kernel::dma_alp* dalp = &top->alps[1];
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
        intf->intf_dbg("snd window empty snd_nxt %u snd_una %u snd_wnd %u\n",
                       snd_nxt,snd_una,snd_wnd);
        return;
    }

    while (!unsent_send_ops.empty() && snd_nxt != snd_una + snd_wnd)
    {
        tcp::send_op* sop = klist_front(unsent_send_ops,link);
        tcp::tx_op* top   = make_one_packet(sop);
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
            if (!sop->refcount)
            {
                sop->cb(sop);
                send_ops_slab.free(sop);
            }
            else
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
}

void
tcp::socket::rx_append(net::rx_page* p)
{
    rx_pages.push_back(&p->link);
    rx_avail_bytes += p->client_len;
    if (state >= TCP_ESTABLISHED)
        observer->socket_readable(this);
}

void
tcp::socket::read(void* _dst, uint32_t rem)
{
    kassert(rem <= rx_avail_bytes);
    rx_avail_bytes -= rem;

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
            dump_socket();
            observer->socket_established(this);
            if (rx_avail_bytes)
                observer->socket_readable(this);
        break;

        default:
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
            TRANSITION(TCP_CLOSED);
        break;

        default:
            kernel::panic("Bad state for syn_send_op_cb!");
        break;
    }
}

void
tcp::socket::handle_retransmit_expiry(kernel::timer_entry*)
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
tcp::socket::handle_time_wait_expiry(kernel::timer_entry* wqe)
{
    kassert(state == TCP_TIME_WAIT);
    TRANSITION(TCP_CLOSED);
    intf->tcp_unlink(this);
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
                throw header_invalid_exception();
            if (h->tcp.syn)
            {
                rx_opts = h->tcp.parse_options();
                process_options(rx_opts);

                snd_wnd = h->tcp.window_size;
                rcv_nxt = h->tcp.seq_num + 1;

                post_ack(snd_nxt,rcv_nxt,rcv_wnd,rcv_wnd_shift);

                if (state == TCP_SYN_SENT_ACKED_WAIT_SYN)
                {
                    TRANSITION(TCP_ESTABLISHED);
                    dump_socket();
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
            process_header_synchronized(h);
            if (h->tcp.fin)
                throw fin_recvd_exception{0};
        break;

        case TCP_LAST_ACK:
            process_header_synchronized(h);
            if (state == TCP_CLOSED)
            {
                intf->tcp_unlink(this);
                observer->socket_closed(this);
            }
        break;

        case TCP_CLOSED:
            intf->intf_dbg("dropping packet\n");
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

        case TCP_TIME_WAIT:
            kernel::cpu::cancel_timer(&time_wait_wqe);
        case TCP_FIN_WAIT_2:
            TRANSITION(TCP_TIME_WAIT);
            kernel::cpu::schedule_timer_sec(&time_wait_wqe,
                                            TIME_WAIT_TIMEOUT_SEC);
        break;

        default:
            kernel::panic("Impossible!");
        break;
    }
    observer->socket_fin_recvd(this);
    return e.flags;
}
catch (socket_reset_exception)
{
    TRANSITION(TCP_CLOSED);
    intf->tcp_unlink(this);
    observer->socket_reset(this);
    return 0;
}
catch (header_invalid_exception)
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
            if (!h->tcp.rst)
                post_rst(h->tcp.ack_num);
        break;

        default:
        break;
    }
    return 0;
}
catch (option_parse_exception& e)
{
    intf->intf_dbg("option parse error: %s (%lu)\n",e.msg,e.val);
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
            TRANSITION(TCP_CLOSED);
            intf->tcp_unlink(this);
            observer->socket_closed(this);
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
tcp::socket::process_header_synchronized(const ipv4_tcp_headers* h)
{
    // We're in a state where we've received the initial SYN so we need to
    // check sequence numbers, various flags and handle ACKs to advance the
    // send queue.
    if (!seq_check(rcv_nxt,h->tcp.seq_num,h->segment_len(),rcv_wnd))
    {
        if (!h->tcp.rst)
            post_ack(snd_nxt,rcv_nxt,rcv_wnd,rcv_wnd_shift);
        throw header_invalid_exception();
    }
    if (h->tcp.rst)
        throw socket_reset_exception();
    if (h->tcp.syn)
    {
        post_rst(snd_nxt);
        throw socket_reset_exception();
    }
    if (!h->tcp.ack)
        throw header_invalid_exception();

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

    if (new_seqs.len)
    {
        rcv_nxt += new_seqs.len;
        post_ack(snd_nxt,rcv_nxt,rcv_wnd,rcv_wnd_shift);
        uint32_t skip = new_seqs.first - h->tcp.seq_num;
        p->client_len = h->payload_len() - skip;
        if (p->client_len)
        {
            p->client_offset = (uint8_t*)h->get_payload() - p->payload + skip;
            rx_append(p);
            if (fin)
                throw fin_recvd_exception{NRX_FLAG_NO_DELETE};
            return NRX_FLAG_NO_DELETE;
        }
    }

    if (fin)
        throw fin_recvd_exception{0};

    return 0;
}

void
tcp::socket::process_options(parsed_options opts)
{
    if (opts.flags & OPTION_SND_MSS_PRESENT)
        snd_mss = MIN(opts.snd_mss,intf->tx_mtu - sizeof(ipv4_tcp_headers));
    if (opts.flags & OPTION_SND_WND_SHIFT_PRESENT)
    {
        snd_wnd_shift = opts.snd_wnd_shift;
        rcv_wnd_shift = RX_WINDOW_SHIFT;
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
