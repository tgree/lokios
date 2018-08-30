#ifndef __KERNEL_NET_NET_H
#define __KERNEL_NET_NET_H

#include "k++/klist.h"
#include "k++/delegate.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "kernel/schedule.h"

namespace net
{
    struct interface;

    // Parameters.
#define TX_COMPLETION_DELAY_10MS    0

    // Transmit parameter block.
#define NTX_FLAG_INSERT_IP_CSUM     (1<<0)
#define NTX_FLAG_INSERT_UDP_CSUM    (1<<1)
#define NTX_FLAG_INSERT_TCP_CSUM    (1<<2)
    struct tx_op
    {
        kernel::klink                   link;
        kernel::delegate<void(tx_op*)>  cb;
#if TX_COMPLETION_DELAY_10MS
        kernel::timer_entry             delay_wqe;
#endif
        uint32_t                        flags;
        uint32_t                        nalps;
        kernel::dma_alp                 alps[6];
    };
#if TX_COMPLETION_DELAY_10MS
    KASSERT(sizeof(tx_op) == 192);
#else
    KASSERT(sizeof(tx_op) == 128);
#endif

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

    private:
        rx_page() = default;
        ~rx_page() = default;

        friend class net::interface;
    };
    KASSERT(sizeof(rx_page) == PAGE_SIZE);

    struct observer
    {
        kernel::kdlink  link;

        // Called when the interface becomes ready for use after being created.
        // The link state may not yet be known.
        virtual void intf_activated(net::interface* intf) {}

        // Called when the link comes up.
        virtual void intf_link_up(net::interface* intf) {}

        // Called when the link goes down.
        virtual void intf_link_down(net::interface* intf) {}

        // Called when the interface is about to become not-ready for use, just
        // before being destroyed.
        virtual void intf_deactivated(net::interface* intf) {}

        // Constructor.
        observer();
    };
}

#endif /* __KERNEL_NET_NET_H */
