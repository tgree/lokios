#ifndef __KERNEL_NET_ETH_ADDR_H
#define __KERNEL_NET_ETH_ADDR_H

#include <stdint.h>
#include <stddef.h>

namespace eth
{
    struct addr
    {
        uint8_t v[6];

        void operator=(uint64_t _v)
        {
            v[0] = (_v >> 40);
            v[1] = (_v >> 32);
            v[2] = (_v >> 24);
            v[3] = (_v >> 16);
            v[4] = (_v >>  8);
            v[5] = (_v >>  0);
        }
        uint8_t operator[](size_t i) const {return v[i];}
    };
    constexpr bool operator==(const addr& lhs, const addr& rhs)
    {
        return lhs.v[0] == rhs.v[0] &&
               lhs.v[1] == rhs.v[1] &&
               lhs.v[2] == rhs.v[2] &&
               lhs.v[3] == rhs.v[3] &&
               lhs.v[4] == rhs.v[4] &&
               lhs.v[5] == rhs.v[5];
    }
    constexpr bool operator!=(const addr& lhs, const addr& rhs)
    {
        return !(lhs == rhs);
    }

    constexpr const addr broadcast_addr{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
}

#include "k++/hash.h"
namespace hash
{
    template<>
    inline size_t compute(const eth::addr& k)
    {
        return ((uint64_t)k[0] << 40) |
               ((uint64_t)k[1] << 32) |
               ((uint64_t)k[2] << 24) |
               ((uint64_t)k[3] << 16) |
               ((uint64_t)k[4] <<  8) |
               ((uint64_t)k[5] <<  0);
    }
}

#endif /* __KERNEL_NET_ETH_ADDR_H */
