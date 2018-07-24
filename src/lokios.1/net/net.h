#ifndef __KERNEL_NET_NET_H
#define __KERNEL_NET_NET_H

#include "k++/klist.h"
#include "mm/mm.h"
#include "mm/page.h"

namespace net
{
    // Transmit parameter block.
    struct tx_op
    {
        kernel::klink   link;
        void           (*cb)(tx_op* op);
        uint64_t        rsrv;
        size_t          nalps;
        kernel::dma_alp alps[2];
    };
    KASSERT(sizeof(tx_op) == 64);

    // Receive parameter block.  The pay_offset and pay_len fields are the
    // offsets from the start of the payload to the beginning of the Link-Layer
    // frame (some drivers may require driver-specific headers preceding the
    // Ethernet part) and the length of the Link-Layer frame (not including any
    // driver-specific headers).
    //
    // Note: the pay_len field is the length of the entire Link-Layer frame.
    // The minimum payload size for an Ethernet frame's payload field is 46 (or
    // 42, if 802.1Q tagging is enabled), so the payload field may be padded on
    // the send side for short packets.  In other words, the pay_len field is
    // an upper bound on the usable payload, but not a tight upper bound.
    //
    // In short: ignore everything before payload + pay_offset and don't trust
    // that everything from payload + pay_offset + pay_len is actually client
    // data.
    struct rx_page
    {
        kernel::klink   link;
        uint16_t        pay_offset;
        uint16_t        pay_len;
        uint8_t         payload[4084];
    };
    KASSERT(sizeof(rx_page) == PAGE_SIZE);

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

#endif /* __KERNEL_NET_NET_H */
