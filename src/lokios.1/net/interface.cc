#include "interface.h"
#include "tcp/traits.h"
#include "tcp/tcp.h"
#include "udp/udp.h"
#include "kernel/console.h"
#include "kernel/cpu.h"
#include "mm/vm.h"
#include "mm/slab.h"
#include "k++/kmath.h"

using kernel::_kassert;

static kernel::spinlock ids_lock;
static uint32_t free_ids = 0xFFFFFFFF;

static size_t
alloc_id()
{
    with (ids_lock)
    {
        kassert(free_ids != 0);
        size_t id = kernel::ffs(free_ids);
        free_ids &= ~(1<<id);
        return id;
    }
}

static void
free_id(size_t id)
{
    with (ids_lock)
    {
        free_ids |= (1<<id);
    }
}

net::interface::interface(size_t tx_qlen, size_t rx_qlen, uint16_t tx_mtu,
    uint16_t rx_mtu):
        id(alloc_id()),
        tx_qlen(tx_qlen),
        rx_qlen(rx_qlen),
        rx_posted_count(0),
        ip_addr{0,0,0,0},
        tx_mtu(tx_mtu),
        rx_mtu(rx_mtu),
        tcp_ephemeral_ports((uint16_t*)tcp_ephemeral_ports_mem.addr,
                            tcp_ephemeral_ports_mem.len/sizeof(uint16_t)),
        cmd_listener(this)
{
    for (size_t i=FIRST_EPHEMERAL_PORT; i<65536; ++i)
        tcp_ephemeral_ports.emplace_back(i);
}

net::interface::~interface()
{
    free_id(id);
}

void
net::interface::intf_vdbg(const char* fmt, va_list ap)
{
    kernel::console::p2printf(fmt,ap,"net%zu: ",id);
}

void
net::interface::udp_listen(uint16_t port, udp_frame_handler h)
{
    udp_sockets.emplace(port,h);
}

void
net::interface::udp_ignore(uint16_t port)
{
    udp_sockets.erase(port);
}

void
net::interface::tcp_listen(uint16_t port,
    tcp::socket_accepted_delegate ad,
    tcp::should_accept_delegate sad)
{
    kassert(port < FIRST_EPHEMERAL_PORT);
    tcp_listeners.emplace(port,tcp::listener{sad,ad});
}

void
net::interface::tcp_ignore(uint16_t port)
{
    tcp_listeners.erase(port);
}

tcp::socket*
net::interface::tcp_connect(ipv4::addr remote_ip, uint16_t remote_port,
    tcp::socket_observer* observer)
{
    uint16_t local_port = tcp_ephemeral_ports[0];
    tcp_ephemeral_ports.pop_front();
    auto sid = tcp::socket_id{remote_ip,remote_port,local_port};
    return &tcp_sockets.emplace(sid,this,remote_ip,local_port,remote_port,
                                nullptr,0,observer);
}

void
net::interface::tcp_delete(tcp::socket* s)
{
    intf_dbg("deleting socket\n");
    if (s->local_port >= FIRST_EPHEMERAL_PORT)
        tcp_ephemeral_ports.emplace_back(s->local_port);
    tcp_sockets.erase_value(s);
}

void
net::interface::activate()
{
    // Post receive buffers.
    refill_rx_pages();

    // Start the cmd_sock listener.
    cmd_listener.listen(12345);
}

void
net::interface::refill_rx_pages()
{
    kernel::klist<net::rx_page> pages;
    size_t n = rx_qlen - rx_posted_count;
    for (size_t i=0; i<n; ++i)
    {
        auto* p = new net::rx_page;
        pages.push_back(&p->link);
    }
    rx_posted_count = rx_qlen;
    post_rx_pages(pages);
}

void
net::interface::handle_tx_completion(net::tx_op* op)
{
#if TX_COMPLETION_DELAY_10MS
    op->delay_wqe.fn      = timer_delegate(handle_delayed_completion);
    op->delay_wqe.args[0] = (uint64_t)this;
    op->delay_wqe.args[1] = (uint64_t)op;
    kernel::cpu::schedule_timer(&op->delay_wqe,TX_COMPLETION_DELAY_10MS);
#else
    op->cb(op);
#endif
}

void
net::interface::handle_delayed_completion(kernel::timer_entry* wqe)
{
    auto* op = (net::tx_op*)wqe->args[1];
    op->cb(op);
}

uint64_t
net::interface::handle_rx_ipv4_frame(net::rx_page* p)
{
    if (p->pay_len < sizeof(ipv4::header))
        return 0;

    auto* iph = p->payload_cast<ipv4::header*>();
    if (p->pay_len < iph->total_len)
        return 0;

    if (iph->dst_ip != ip_addr && iph->dst_ip != ipv4::broadcast_addr)
        return 0;

    switch (iph->proto)
    {
        case tcp::net_traits::ip_proto:
            return tcp::handle_rx_ipv4_tcp_frame(this,p);

        case udp::net_traits::ip_proto:
            return handle_rx_ipv4_udp_frame(p);
    }

    return 0;
}

uint64_t
net::interface::handle_rx_ipv4_udp_frame(net::rx_page* p)
{
    auto* iph = p->payload_cast<ipv4::header*>();
    auto* uh  = (udp::header*)(iph+1);

    udp_frame_handler* ufh;
    try
    {
        ufh = &udp_sockets[uh->dst_port];
    }
    catch (hash::no_such_key_exception&)
    {
        return 0;
    }

    (*ufh)(this,p);
    return 0;
}

