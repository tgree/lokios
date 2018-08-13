#ifndef __KERNEL_NET_MOCK_FINTERFACE_H
#define __KERNEL_NET_MOCK_FINTERFACE_H

#include "../interface.h"

namespace net
{
    struct finterface : public net::interface
    {
        kernel::klist<net::tx_op>   posted_ops;
        kernel::klist<net::rx_page> posted_pages;

        virtual size_t  format_ll_reply(net::rx_page* p, void* reply_payload);
        virtual size_t  format_arp_broadcast(void* arp_payload);
        virtual void    post_tx_frame(net::tx_op* op);
        virtual void    post_rx_pages(kernel::klist<net::rx_page>& pages);
        virtual void    dump_arp_table();

        net::tx_op*     pop_tx_op();
        net::rx_page*   pop_rx_page();
        uint32_t        handle_rx_page(net::rx_page* p);

        finterface(ipv4::addr ip_addr);
        virtual ~finterface();
    };
}

#endif /* __KERNEL_NET_MOCK_FINTERFACE_H */