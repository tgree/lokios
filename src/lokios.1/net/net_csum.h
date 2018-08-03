#ifndef __KERNEL_NET_NET_CSUM_H
#define __KERNEL_NET_NET_CSUM_H

#include <stdint.h>
#include <stddef.h>

namespace net
{
    // Incremental one's-complement checksumming function.
    // Highly unoptimized.
    static inline uint16_t ones_comp_csum(const void* _p, size_t len,
                                          size_t offset, uint16_t seed = 0xFFFF)
    {
        auto* p    = (const uint8_t*)_p;
        uint32_t s = (~seed & 0x0000FFFF);
        for (size_t i=offset; i != offset+len; ++i)
        {
            if (i & 1)
                s += *p++;
            else
                s += (*p++ << 8);
        }
        while (s >> 16)
            s = (s >> 16) + (s & 0xFFFF);
        return ~s;
    }
}

#endif /* __KERNEL_NET_NET_CSUM_H */
