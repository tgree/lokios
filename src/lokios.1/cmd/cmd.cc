#include "net/interface.h"
#include "net/wka.h"
#include "kern/task.h"
#include "kern/console.h"
#include "platform/platform.h"

using kernel::_kassert;

struct cmd_sock_connection : public tcp::socket_observer
{
    net::interface*         intf;
    tcp::socket*            s;

    size_t                  buf_len;
    char                    buf[128];

    virtual void    socket_established(tcp::socket* s);
    virtual void    socket_readable(tcp::socket* s);
    virtual void    socket_recv_closed(tcp::socket* s);
    virtual void    socket_closed(tcp::socket* s);
    virtual void    socket_reset(tcp::socket* s);

            void    send_complete(tcp::send_op* sop);

            void    handle_cmd_arp();
            void    handle_cmd_mem();
            void    handle_cmd_panic();
            void    handle_cmd_exit();
            void    handle_cmd_segv();
            void    handle_cmd_close();
            void    handle_unrecognized_cmd();

    cmd_sock_connection(tcp::socket* s);
};

struct cmd_sock_net_observer : public net::observer
{
    static kernel::dma_alp  spam_alps[1];
    kernel::slab            connection_slab;

    void cmd_socket_accepted(tcp::socket* s)
    {
        s->dbg("cmd_sock connect accepted\n");
        s->observer = connection_slab.alloc<cmd_sock_connection>(s);
    }

    virtual void intf_activated(net::interface* intf) override
    {
        intf->tcp_listen(TCP_LOKIOS_CMD_PORT,32768,
                         method_delegate(cmd_socket_accepted));
    }

    cmd_sock_net_observer():
        connection_slab(sizeof(cmd_sock_connection))
    {
        spam_alps[0].paddr = kernel::buddy_palloc(2);
        spam_alps[0].len   = (PAGE_SIZE << 2);

        char* str = (char*)kernel::phys_to_virt(spam_alps[0].paddr);
        memset(str,'X',spam_alps[0].len);
        str[0] = 'T';
        memcpy(str+spam_alps[0].len-11,"\r\nGoodbye\r\n",11);
    }
};

kernel::dma_alp cmd_sock_net_observer::spam_alps[1];
cmd_sock_net_observer cmd_sock_observer;

cmd_sock_connection::cmd_sock_connection(tcp::socket* s):
    intf(s->intf),
    s(s),
    buf_len(0)
{
}

void
cmd_sock_connection::socket_established(tcp::socket* _s)
{
    kassert(_s == s);

    s->dbg("cmd_sock connect established\n");
}

void
cmd_sock_connection::socket_readable(tcp::socket* _s)
{
    kassert(_s == s);

    while (s->rx_avail_bytes)
    {
        size_t rem = sizeof(buf) - 1 - buf_len;
        if (rem == 0)
        {
            s->dbg("command too long, discarding\n");
            buf_len = 0;
            rem     = sizeof(buf);
        }

        uint32_t len = MIN(s->rx_avail_bytes,rem);
        s->read(buf+buf_len,len);
        buf_len += len;
        buf[buf_len] = '\0';

        char* pos = (char*)memmem(buf,buf_len,"\r\n",2);
        if (!pos)
            pos = (char*)memmem(buf,buf_len,"\r",2);
        if (pos)
        {
            *pos = '\0';

            if (!strcmp(buf,"arp"))
                handle_cmd_arp();
            else if (!strcmp(buf,"mem"))
                handle_cmd_mem();
            else if (!strcmp(buf,"panic"))
                handle_cmd_panic();
            else if (!strcmp(buf,"exit"))
                handle_cmd_exit();
            else if (!strcmp(buf,"segv"))
                handle_cmd_segv();
            else if (!strcmp(buf,"close"))
                handle_cmd_close();
            else
                handle_unrecognized_cmd();

            size_t n = pos + 2 - buf;
            buf_len -= n;
            memmove(buf,pos+2,buf_len);
        }
    }
    kassert(s->rx_pages.empty());

    if (s->in_sendable_state())
    {
        s->send(NELEMS(cmd_sock_net_observer::spam_alps),
                cmd_sock_net_observer::spam_alps,
                method_delegate(send_complete));
    }
}

void
cmd_sock_connection::socket_recv_closed(tcp::socket* _s)
{
    kassert(_s == s);
    s->dbg("receive closed\n");
    if (s->in_passive_close_state())
        s->close_send();
}

void
cmd_sock_connection::socket_closed(tcp::socket* _s)
{
    kassert(_s == s);
    s->dbg("connection closed\n");
    intf->tcp_delete(s);
    cmd_sock_observer.connection_slab.free(this);
}

void
cmd_sock_connection::socket_reset(tcp::socket* _s)
{
    kassert(_s == s);
    s->dbg("error: connection reset\n");
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
    s->dbg("Free pages: %zu  PT Used Pages: %zu\n",
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

void
cmd_sock_connection::handle_cmd_close()
{
    s->close_send();
}

void
cmd_sock_connection::handle_unrecognized_cmd()
{
    s->dbg("Unrecognized command\n");
}
