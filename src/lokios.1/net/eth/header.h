#ifndef __KERNEL_NET_ETH_HEADER_H
#define __KERNEL_NET_ETH_HEADER_H

#include "addr.h"

namespace eth
{
    struct header
    {
        typedef eth::addr addr_type;

        eth::addr   dst_mac;
        eth::addr   src_mac;
        be_uint16_t ether_type;
    } __PACKED__;
    KASSERT(sizeof(header) == 14);
}

#endif /* __KERNEL_NET_ETH_HEADER_H */
