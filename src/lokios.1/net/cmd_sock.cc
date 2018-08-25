#include "cmd_sock.h"
#include "interface.h"
#include "kernel/task.h"
#include "platform/platform.h"

using kernel::_kassert;

struct cmd_sock_connection : public tcp::socket_observer
{
    net::cmd_sock_listener* listener;
    net::interface*         intf;
    tcp::socket*            s;

    virtual void    socket_established(tcp::socket* s);
    virtual void    socket_readable(tcp::socket* s);
    virtual void    socket_reset(tcp::socket* s);

            void    send_complete(tcp::send_op* sop);

            void    handle_cmd_arp();
            void    handle_cmd_mem();
            void    handle_cmd_panic();
            void    handle_cmd_exit();
            void    handle_cmd_segv();

    cmd_sock_connection(net::cmd_sock_listener* listener, tcp::socket* s);
};

static kernel::dma_alp spam_alps[1];

net::cmd_sock_listener::cmd_sock_listener(net::interface* intf):
    intf(intf),
    connection_slab(sizeof(cmd_sock_connection))
{
    if (!spam_alps[0].paddr)
    {
        spam_alps[0].paddr = kernel::buddy_palloc(2);
        spam_alps[0].len   = (PAGE_SIZE << 2);

        char* str = (char*)kernel::phys_to_virt(spam_alps[0].paddr);
        memset(str,'X',spam_alps[0].len);
        str[0] = 'T';
        memcpy(str+spam_alps[0].len-11,"\r\nGoodbye\r\n",11);
    }
}

void
net::cmd_sock_listener::listen(uint16_t port)
{
    intf->tcp_listen(port,method_delegate(cmd_socket_accepted));
}

void
net::cmd_sock_listener::cmd_socket_accepted(tcp::socket* s)
{
    intf->intf_dbg("cmd_sock connect from %u.%u.%u.%u:%u accepted\n",
                   s->remote_ip[0],
                   s->remote_ip[1],
                   s->remote_ip[2],
                   s->remote_ip[3],
                   (uint16_t)s->remote_port);
    s->observer = connection_slab.alloc<cmd_sock_connection>(this,s);
}

cmd_sock_connection::cmd_sock_connection(net::cmd_sock_listener* listener,
    tcp::socket* s):
        listener(listener),
        intf(listener->intf),
        s(s)
{
}

void
cmd_sock_connection::socket_established(tcp::socket* _s)
{
    kassert(_s == s);

    intf->intf_dbg("cmd_sock connect from %u.%u.%u.%u:%u established\n",
                   s->remote_ip[0],
                   s->remote_ip[1],
                   s->remote_ip[2],
                   s->remote_ip[3],
                   (uint16_t)s->remote_port);
}

void
cmd_sock_connection::socket_readable(tcp::socket* _s)
{
    kassert(_s == s);

    char buffer[16];
    memset(buffer,'T',sizeof(buffer));
    while (s->rx_avail_bytes)
    {
        uint32_t len = MIN(s->rx_avail_bytes,sizeof(buffer)-1);
        buffer[len]  = '\0';
        s->read(buffer,len);
        if (!strcmp(buffer,"arp\r\n"))
            handle_cmd_arp();
        else if (!strcmp(buffer,"mem\r\n"))
            handle_cmd_mem();
        else if (!strcmp(buffer,"panic\r\n"))
            handle_cmd_panic();
        else if (!strcmp(buffer,"exit\r\n"))
            handle_cmd_exit();
        else if (!strcmp(buffer,"segv\r\n"))
            handle_cmd_segv();
    }
    kassert(s->rx_pages.empty());

    s->send(NELEMS(spam_alps),spam_alps,method_delegate(send_complete));
}

void
cmd_sock_connection::socket_reset(tcp::socket* _s)
{
    kassert(_s == s);
    intf->tcp_delete(s);
    listener->connection_slab.free(this);
}

void
cmd_sock_connection::send_complete(tcp::send_op* sop)
{
}

void
cmd_sock_connection::handle_cmd_arp()
{
    intf->dump_arp_table();
}

void
cmd_sock_connection::handle_cmd_mem()
{
    intf->intf_dbg("Free pages: %zu  PT Used Pages: %zu\n",
                   kernel::page_count_free(),
                   kernel::kernel_task->pt.page_count);
}

void
cmd_sock_connection::handle_cmd_panic()
{
    kernel::panic("User requested");
}

void
cmd_sock_connection::handle_cmd_exit()
{
    kernel::exit_guest(1);
}

void
cmd_sock_connection::handle_cmd_segv()
{
    *(volatile uint64_t*)0x123 = 0x4567890ABCDEF123ULL;
}
