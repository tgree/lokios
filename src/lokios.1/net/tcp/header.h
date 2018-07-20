#ifndef __KERNEL_NET_TCP_HEADER_H
#define __KERNEL_NET_TCP_HEADER_H

#include "kernel/types.h"

namespace tcp
{
    struct header
    {
        be_uint16_t src_port;
        be_uint16_t dst_port;
        be_uint32_t seq_num;
        be_uint32_t ack_num;
        uint16_t    fin    : 1,
                    syn    : 1,
                    rst    : 1,
                    psh    : 1,
                    ack    : 1,
                    urg    : 1,
                    ece    : 1,
                    cwr    : 1,
                    ns     : 1,
                    rsrv   : 3,
                    offset : 4;
        be_uint16_t window_size;
        be_uint16_t checksum;
        be_uint16_t urgent_pointer;
    } __PACKED__;
    KASSERT(sizeof(header) == 20);
}

#endif /* __KERNEL_NET_TCP_HEADER_H */
