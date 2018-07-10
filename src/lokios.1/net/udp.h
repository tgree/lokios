#ifndef __KERNEL_NET_UDP_H
#define __KERNEL_NET_UDP_H

#include "hdr/compiler.h"
#include "kernel/kassert.h"
#include <stdint.h>

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
}

#endif /* __KERNEL_NET_UDP_H */
