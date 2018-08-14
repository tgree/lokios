#include "cmd_sock.h"
#include "interface.h"
#include "kernel/task.h"
#include "platform/platform.h"

using kernel::_kassert;

static kernel::dma_alp spam_alps[1];

net::cmd_sock_listener::cmd_sock_listener(net::interface* intf):
    intf(intf)
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
    s->observer = this;
}

void
net::cmd_sock_listener::socket_established(tcp::socket* s)
{
    intf->intf_dbg("cmd_sock connect from %u.%u.%u.%u:%u established\n",
                   s->remote_ip[0],
                   s->remote_ip[1],
                   s->remote_ip[2],
                   s->remote_ip[3],
                   (uint16_t)s->remote_port);
}

void
net::cmd_sock_listener::socket_readable(tcp::socket* s)
{
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
    }
    kassert(s->rx_pages.empty());

    s->send(NELEMS(spam_alps),spam_alps,method_delegate(send_complete));
}

void
net::cmd_sock_listener::socket_reset(tcp::socket* s)
{
    intf->tcp_delete(s);
}

void
net::cmd_sock_listener::send_complete(tcp::send_op* sop)
{
}

void
net::cmd_sock_listener::handle_cmd_arp()
{
    intf->dump_arp_table();
}

void
net::cmd_sock_listener::handle_cmd_mem()
{
    intf->intf_dbg("Free pages: %zu  PT Used Pages: %zu\n",
                   kernel::page_count_free(),
                   kernel::kernel_task->pt.page_count);
}

void
net::cmd_sock_listener::handle_cmd_panic()
{
    kernel::panic("User requested");
}

void
net::cmd_sock_listener::handle_cmd_exit()
{
    kernel::exit_guest(1);
}
