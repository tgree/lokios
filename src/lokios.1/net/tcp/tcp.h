#ifndef __KERNEL_NET_TCP_TCP_H
#define __KERNEL_NET_TCP_TCP_H

#include "header.h"
#include "net/net.h"

#define MAX_TX_OPTIONS_SIZE    16

namespace net
{
    struct interface;
}

namespace tcp
{
    struct tx_op : public net::tx_op
    {
        kernel::klink       tcp_link;

        uint8_t             llhdr[16];
        ipv4_tcp_headers    hdrs;
        uint8_t             options[MAX_TX_OPTIONS_SIZE];

        void    format_reply(net::interface* intf, net::rx_page* p);
        void    format_rst(uint32_t seq_num);
        void    format_rst_ack(uint32_t ack_num);
        void    format_ack(uint32_t seq_num, uint32_t ack_num,
                           uint16_t window_size);
    };
    KASSERT(offsetof(tx_op,hdrs) ==
            offsetof(tx_op,llhdr) + sizeof_field(tx_op,llhdr));

    // Packet handling.
    uint64_t handle_rx_ipv4_tcp_frame(net::interface* intf, net::rx_page* p);
}

#endif /* __KERNEL_NET_TCP_TCP_H */
