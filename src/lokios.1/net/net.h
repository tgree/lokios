#ifndef __KERNEL_NET_NET_H
#define __KERNEL_NET_NET_H

#include "k++/klist.h"
#include "k++/delegate.h"
#include "mm/mm.h"
#include "mm/page.h"

namespace net
{
    // Transmit parameter block.
#define NTX_FLAG_INSERT_IP_CSUM     (1<<0)
#define NTX_FLAG_INSERT_UDP_CSUM    (1<<1)
#define NTX_FLAG_INSERT_TCP_CSUM    (1<<2)
    struct tx_op
    {
        kernel::klink                   link;
        kernel::delegate<void(tx_op*)>  cb;
        uint32_t                        flags;
        uint32_t                        nalps;
        kernel::dma_alp                 alps[2];
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
#define NRX_FLAG_NO_DELETE  (1<<0)  // Set by client to prevent auto-delete.
    struct rx_page
    {
        kernel::klink   link;
        uint16_t        pay_offset;
        uint16_t        pay_len;
        uint16_t        client_offset;
        uint16_t        client_len;
        uint8_t         payload[4080];

        // Get a pointer to the payload in a stripped page.
        template<typename T>
        inline T payload_cast()
        {
            return (T)(payload + pay_offset);
        }

        // Get a pointer to the link-layer header in a stripped page.
        template<typename T>
        inline T llhdr_cast()
        {
            return (T)(payload + pay_offset - sizeof(*(T)0));
        }
    };
    KASSERT(sizeof(rx_page) == PAGE_SIZE);
}

#endif /* __KERNEL_NET_NET_H */
