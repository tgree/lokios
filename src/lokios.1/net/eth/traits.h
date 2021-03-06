#ifndef __KERNEL_NET_ETH_TRAITS_H
#define __KERNEL_NET_ETH_TRAITS_H

#include "addr.h"
#include "header.h"
#include "interface.h"

namespace eth
{
    struct net_traits
    {
        typedef eth::addr       addr_type;
        typedef eth::header     header_type;
        typedef eth::interface  interface_type;

        static constexpr const uint16_t arp_hw_type = 1;
        static constexpr const eth::addr broadcast_addr{0xFF,0xFF,0xFF,
                                                        0xFF,0xFF,0xFF};
        static constexpr const eth::addr zero_addr{0,0,0,0,0,0};
    };
}

#endif /* __KERNEL_NET_ETH_TRAITS_H */
