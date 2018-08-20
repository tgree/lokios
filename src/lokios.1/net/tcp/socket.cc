#include "socket.h"
#include "traits.h"
#include "net/interface.h"
#include "k++/random.h"
#include "kernel/cpu.h"

#define RETRANSMIT_TIMEOUT_MS   1000

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

static void
syn_sent_send_op_cb(tcp::send_op*)
{
}

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

tcp::socket::socket(net::interface* intf, net::rx_page* p):
    intf(intf),
    state(TCP_LISTEN),
    prev_state(TCP_CLOSED),
    llsize(intf->format_ll_reply(p,&llhdr,sizeof(llhdr))),
    remote_ip(p->payload_cast<tcp::ipv4_tcp_headers*>()->ip.src_ip),
    local_port(p->payload_cast<tcp::ipv4_tcp_headers*>()->tcp.dst_port),
    remote_port(p->payload_cast<tcp::ipv4_tcp_headers*>()->tcp.src_port),
    observer(NULL),
    tx_ops_slab(sizeof(tcp::tx_op)),
    send_ops_slab(sizeof(tcp::send_op)),
    rx_avail_bytes(0)
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
    snd_nxt       = iss;
    snd_wnd       = 0;
    snd_wnd_shift = 0;
    snd_mss       = MIN(MAX_SAFE_IP_SIZE,intf->tx_mtu) -
                    sizeof(ipv4_tcp_headers);

    irs           = 0;
    rcv_nxt       = 0;
    rcv_wnd       = MAX_RX_WINDOW;
    rcv_wnd_shift = 0;
    rcv_mss       = intf->rx_mtu - sizeof(ipv4_tcp_headers);
}

tcp::socket::socket(net::interface* intf, ipv4::addr remote_ip,
    uint16_t local_port, uint16_t remote_port, const void* llh,
    size_t llsize, socket_observer* observer):
        intf(intf),
        state(TCP_CLOSED),
        prev_state(TCP_CLOSED),
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

    iss           = kernel::random(0,0xFFFF);
    snd_una       = iss;
    snd_nxt       = iss;
    snd_wnd       = 1;
    snd_wnd_shift = 0;
    snd_mss       = MIN(MAX_SAFE_IP_SIZE,intf->tx_mtu) -
                    sizeof(ipv4_tcp_headers);

    irs           = 0;
    rcv_nxt       = 0;
    rcv_wnd       = MAX_RX_WINDOW;
    rcv_wnd_shift = RX_WINDOW_SHIFT;
    rcv_mss       = intf->rx_mtu - sizeof(ipv4_tcp_headers);

    send(0,NULL,kernel::func_delegate(syn_sent_send_op_cb),
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
        case TCP_LISTEN:
            kassert(flags & SEND_OP_FLAG_SET_ACK);
        case TCP_CLOSED:
            kassert(flags & SEND_OP_FLAG_SYN);
        case TCP_ESTABLISHED:
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
        --avail_len;

        auto* opt                = top->hdrs.tcp.options;
        opt[0]                   = 2;
        opt[1]                   = 4;
        *(be_uint16_t*)(opt + 2) = rcv_mss;
        opt                     += 4;

        if (sop->flags & SEND_OP_FLAG_SET_SCALE)
        {
            opt[0] = 3;
            opt[1] = 3;
            opt[2] = rcv_wnd_shift;
            opt[3] = 1;
            opt   += 4;
        }

        size_t opt_len          = opt - top->hdrs.tcp.options;
        top->hdrs.ip.total_len += opt_len;
        top->hdrs.tcp.offset   += opt_len/4;
        top->alps[0].len       += opt_len;
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
    if (seq_ge(snd_nxt,snd_una + snd_wnd))
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

uint64_t
tcp::socket::handle_rx_ipv4_tcp_frame(net::rx_page* p)
{
    auto* h = p->payload_cast<ipv4_tcp_headers*>();
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

        case TCP_SYN_RECVD:
            if (!seq_check(rcv_nxt,h->tcp.seq_num,h->segment_len(),rcv_wnd))
            {
                if (!h->tcp.rst)
                    post_ack(snd_nxt,rcv_nxt,rcv_wnd,rcv_wnd_shift);
                break;
            }
            if (h->tcp.rst)
            {
                if (prev_state == TCP_LISTEN)
                    intf->intf_dbg("passive open failed, deleting socket\n");
                else
                    kernel::panic("active open failed - notify client!\n");
                TRANSITION(TCP_CLOSED);
                intf->tcp_delete(this);
                break;
            }
            if (h->tcp.syn)
            {
                // We're just going to close the socket; the packet could get
                // dropped.  It seems like this is unnecessary.
                post_rst(snd_nxt);
                TRANSITION(TCP_CLOSED);
                intf->tcp_delete(this);
                break;
            }
            if (!h->tcp.ack)
                break;
            if (seq_lt(snd_una,h->tcp.ack_num) &&
                seq_le(h->tcp.ack_num,snd_nxt))
            {
                process_ack(h->tcp.ack_num);
                TRANSITION(TCP_ESTABLISHED);
                observer->socket_established(this);
                return handle_established_segment_recvd(p);
            }
            if (h->tcp.fin)
            {
                process_fin(h->tcp.seq_num);
                TRANSITION(TCP_CLOSE_WAIT);
                break;
            }

            // We're just going to close the socket; the packet could
            // get dropped.  It seems like this is unnecessary.
            post_rst(snd_nxt);
            TRANSITION(TCP_CLOSED);
            intf->tcp_delete(this);
        break;

        case TCP_SYN_SENT:
            if (h->tcp.ack)
            {
                if (seq_le(h->tcp.ack_num,iss) ||
                    seq_gt(h->tcp.ack_num,snd_nxt))
                {
                    post_rst(h->tcp.ack_num);
                    break;
                }
                if (h->tcp.rst)
                {
                    intf->intf_dbg("error: connection reset\n");
                    TRANSITION(TCP_CLOSED);
                    intf->tcp_delete(this);
                    break;
                }
                process_ack(h->tcp.ack_num);
                if (!h->tcp.syn)
                    break;

                snd_wnd = h->tcp.window_size;
                parsed_options opts;
                if (parse_options(h,&opts))
                {
                    intf->intf_dbg("error: bad options\n");
                    break;
                }
                if (opts.flags & OPTION_SND_MSS_PRESENT)
                {
                    snd_mss = MIN(opts.snd_mss,
                                  intf->tx_mtu - sizeof(ipv4_tcp_headers));
                }
                if (opts.flags & OPTION_SND_WND_SHIFT_PRESENT)
                    snd_wnd_shift = opts.snd_wnd_shift;
                else
                    rcv_wnd_shift = 0;

                irs     = h->tcp.seq_num;
                rcv_nxt = irs + 1;
                if (seq_gt(snd_una,iss))
                {
                    post_ack(snd_nxt,rcv_nxt,rcv_wnd,rcv_wnd_shift);
                    TRANSITION(TCP_ESTABLISHED);
                    observer->socket_established(this);
                    break;
                }
            }
            else if (h->tcp.rst || !h->tcp.syn)
                break;
            return handle_listen_syn_recvd(p);
        break;

        case TCP_ESTABLISHED:
            return handle_established_segment_recvd(p);
        break;

        case TCP_CLOSED:
        case TCP_CLOSE_WAIT:
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
    parsed_options opts;
    if (parse_options(h,&opts))
        return 0;
    if (opts.flags & OPTION_SND_MSS_PRESENT)
        snd_mss = MIN(opts.snd_mss,intf->tx_mtu - sizeof(ipv4_tcp_headers));
    if (opts.flags & OPTION_SND_WND_SHIFT_PRESENT)
        snd_wnd_shift = opts.snd_wnd_shift;

    // Segment(SEQ=ISS,ACK=RCV.NXT,CTL=SYN/ACK)
    if (opts.flags & OPTION_SND_WND_SHIFT_PRESENT)
    {
        rcv_wnd_shift = RX_WINDOW_SHIFT;
        send(0,NULL,kernel::func_delegate(syn_sent_send_op_cb),
             SEND_OP_FLAG_SYN | SEND_OP_FLAG_SET_ACK | SEND_OP_FLAG_SET_SCALE);
    }
    else
    {
        send(0,NULL,kernel::func_delegate(syn_sent_send_op_cb),
             SEND_OP_FLAG_SYN | SEND_OP_FLAG_SET_ACK);
    }

    TRANSITION(TCP_SYN_RECVD);
    return 0;
}

uint64_t
tcp::socket::handle_established_segment_recvd(net::rx_page* p)
{
    auto* h = p->payload_cast<ipv4_tcp_headers*>();
    if (!seq_check(rcv_nxt,h->tcp.seq_num,h->segment_len(),rcv_wnd))
    {
        if (!h->tcp.rst)
            post_ack(snd_nxt,rcv_nxt,rcv_wnd,rcv_wnd_shift);
        return 0;
    }
    if (h->tcp.rst)
    {
        TRANSITION(TCP_CLOSED);
        intf->tcp_delete(this);
        return 0;
    }
    if (h->tcp.syn)
    {
        post_rst(snd_nxt);
        TRANSITION(TCP_CLOSED);
        intf->tcp_delete(this);
        return 0;
    }
    // ES2:
    if (!h->tcp.ack)
        return 0;
    if (seq_lt(snd_una,h->tcp.ack_num) &&
        seq_le(h->tcp.ack_num,snd_nxt))
    {
        snd_wnd = h->tcp.window_size << snd_wnd_shift;
        process_ack(h->tcp.ack_num);
        process_send_queue();
        // TODO: "Release REXMT timer"?????
    }
    else if (seq_ne(h->tcp.ack_num,snd_una))
        return 0;

    // ES3:
    uint64_t flags   = 0;
    bool fin         = h->tcp.fin;
    uint32_t fin_seq = h->tcp.seq_num;
    if (h->segment_len()) // TERRY ADDED THIS CHECK TO SUPPRESS DUPLICATE ACKS
    {
        if (h->tcp.seq_num == rcv_nxt)
        {
            // TODO: Consume from OO RCV Buffer here.
            rcv_nxt = h->tcp.seq_num + h->segment_len();
            post_ack(snd_nxt,rcv_nxt,rcv_wnd,rcv_wnd_shift);

            // BAH!  The packet can get deleted if we let rx_append call out
            // to the client in context.
            p->client_offset = (uint8_t*)h->get_payload() - p->payload;
            p->client_len    = h->payload_len();
            flags            = NRX_FLAG_NO_DELETE;
            rx_append(p);

            // Though shalt not touch packet memory again.
            h = NULL;
            p = NULL;
        }
        else
            post_ack(snd_nxt,rcv_nxt,rcv_wnd,rcv_wnd_shift);
    }
    if (fin)
    {
        process_fin(fin_seq);
        TRANSITION(TCP_CLOSE_WAIT);
        return flags;
    }

    return flags;
}

void
tcp::socket::rx_append(net::rx_page* p)
{
    rx_pages.push_back(&p->link);
    rx_avail_bytes += p->client_len;
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

void
tcp::socket::process_fin(uint32_t seq_num)
{
    rcv_nxt = seq_num + 1;
    post_ack(snd_nxt,rcv_nxt,rcv_wnd,rcv_wnd_shift);
}

void
tcp::socket::process_ack(uint32_t ack_num)
{
    // snd_una is the first unacknowledged byte and corresponds to the head of
    // the sent_send_ops queue.
    //
    // ack_num is the first unseen sequence number by the remote guy.  So he
    // is acking [snd_una,ack_num).  I.e. sequence number ack_num is NOT being
    // acknowledged yet.
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

int
tcp::socket::parse_options(ipv4_tcp_headers* h, parsed_options* opts)
{
    size_t rem     = h->tcp.offset*4 - sizeof(tcp::header);
    uint8_t* opt   = h->tcp.options;
    uint8_t* start = opt;
    opts->flags    = 0;
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
                    return -1;
                }
                if (opt[1] != 4)
                {
                    intf->intf_dbg("bad MSS length of %u\n",opt[1]);
                    return -2;
                }
                opts->flags  |= OPTION_SND_MSS_PRESENT;
                opts->snd_mss = *(be_uint16_t*)(opt + 2);
                intf->intf_dbg("send MSS: %u\n",opts->snd_mss);
                rem -= 4;
                opt += 4;
            break;

            case 3:     // Window Size Shift
                intf->intf_dbg("%zu: opt %u - window size shift\n",
                               opt-start,*opt);
                if (rem < 3)
                {
                    intf->intf_dbg("truncated WND_SHIFT option\n");
                    return -3;
                }
                if (opt[1] != 3)
                {
                    intf->intf_dbg("bad WND_SHIFT length of %u\n",opt[1]);
                    return -4;
                }
                opts->flags        |= OPTION_SND_WND_SHIFT_PRESENT;
                opts->snd_wnd_shift = opt[2];
                if (opts->snd_wnd_shift > 14)
                {
                    intf->intf_dbg("large WND_SHIFT value of %u, using 14\n",
                                   opts->snd_wnd_shift);
                    opts->snd_wnd_shift = 14;
                }
                rem -= 3;
                opt += 3;
            break;

            default:    // Anything else.
                if (rem < 2)
                {
                    intf->intf_dbg("option %u missing length\n",opt[0]);
                    return -5;
                }
                intf->intf_dbg("%zu: opt %u - other len %u\n",
                               opt-start,*opt,opt[1]);
                if (rem < opt[1])
                {
                    intf->intf_dbg("option %u truncated\n",opt[0]);
                    return -6;
                }
                rem -= opt[1];
                opt += opt[1];
            break;
        }
    }

    return 0;
}
