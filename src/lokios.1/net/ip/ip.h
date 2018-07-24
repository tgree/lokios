#ifndef __KERNEL_NET_IP_H
#define __KERNEL_NET_IP_H

#include "hdr/compiler.h"
#include "kernel/types.h"
#include <stdint.h>
#include <stddef.h>

namespace ipv4
{
    struct addr
    {
        uint8_t v[4];

        void operator=(uint32_t _v)
        {
            v[0] = (_v >> 24);
            v[1] = (_v >> 16);
            v[2] = (_v >>  8);
            v[3] = (_v >>  0);
        }
        uint8_t operator[](size_t i) const {return v[i];}
    };
    constexpr bool operator==(const addr& lhs, const addr& rhs)
    {
        return lhs.v[0] == rhs.v[0] &&
               lhs.v[1] == rhs.v[1] &&
               lhs.v[2] == rhs.v[2] &&
               lhs.v[3] == rhs.v[3];
    }
    constexpr bool operator!=(const addr& lhs, const addr& rhs)
    {
        return !(lhs == rhs);
    }

    constexpr const addr broadcast_addr{255,255,255,255};

    struct header
    {
        uint8_t         version_ihl;
        uint8_t         dscp_ecn;
        be_uint16_t     total_len;
        be_uint16_t     identification;
        be_uint16_t     flags_fragoffset;
        uint8_t         ttl;
        uint8_t         proto;
        be_uint16_t     header_checksum;
        ipv4::addr      src_ip;
        ipv4::addr      dst_ip;
    } __PACKED__;
    KASSERT(sizeof(header) == 20);

    static inline uint16_t csum(header* h)
    {
        size_t nwords     = (h->version_ihl & 0x0F)*2;
        uint32_t s        = 0;
        const uint16_t* p = (const uint16_t*)h;
        for (size_t i=0; i<nwords; ++i)
            s += swap_uint(*p++);
        while (s >> 16)
            s = (s >> 16) + (s & 0xFFFF);
        return ~s;
    }

    struct net_traits
    {
        typedef ipv4::addr      addr_type;
        typedef ipv4::header    header_type;

        static constexpr const uint16_t ether_type = 0x0800;
    };
}

#endif /* __KERNEL_NET_IP_H */
