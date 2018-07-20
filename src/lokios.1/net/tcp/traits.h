#ifndef __KERNEL_NET_TCP_TRAITS_H
#define __KERNEL_NET_TCP_TRAITS_H

#include <stdint.h>

namespace tcp
{
    struct net_traits
    {
        static constexpr const uint8_t ip_proto = 0x06;
    };
}

#endif /* __KERNEL_NET_TCP_TRAITS_H */
