#ifndef __KERNEL_NET_UDP_H
#define __KERNEL_NET_UDP_H

#include "net/net.h"

namespace udp
{
    struct header
    {
        be_uint16_t src_port;
        be_uint16_t dst_port;
        be_uint16_t len;
        be_uint16_t checksum;
    } __PACKED__;
    KASSERT(sizeof(header) == 8);

    struct net_traits
    {
        static constexpr const uint8_t ip_proto = 0x11;
    };

    uint16_t compute_checksum(net::tx_op* op, size_t llhdr_size);
}

#endif /* __KERNEL_NET_UDP_H */
