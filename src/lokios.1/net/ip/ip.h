#ifndef __KERNEL_NET_IP_H
#define __KERNEL_NET_IP_H

#include "net/net_csum.h"
#include "hdr/endian.h"

// All hosts must be prepared to accept a datagram with ip::header::total_len
// of up to 576 octets.  This is the upper bound on what a remote host MUST
// accept, but generally MTUs are around 1500 octets.
#define MAX_SAFE_IP_SIZE    576

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
        return net::ones_comp_csum(h,(h->version_ihl & 0x0F)*4,0);
    }

    struct net_traits
    {
        typedef ipv4::addr      addr_type;
        typedef ipv4::header    header_type;

        static constexpr const uint16_t any_port   = 0;
        static constexpr const uint16_t ether_type = 0x0800;
    };

#define INPORT_ANY  ipv4::net_traits::any_port
}

#include "k++/hash.h"
namespace hash
{
    template<>
    struct hasher<ipv4::addr>
    {
        static inline size_t compute(const ipv4::addr& k)
        {
            return ((uint64_t)k[0] << 24) |
                   ((uint64_t)k[1] << 16) |
                   ((uint64_t)k[2] <<  8) |
                   ((uint64_t)k[3] <<  0);
        }
    };
}

#endif /* __KERNEL_NET_IP_H */
