#ifndef __KERNEL_NET_BCM57762_RING_H
#define __KERNEL_NET_BCM57762_RING_H

#include "kern/types.h"

namespace bcm57762
{
    struct ring_control_block
    {
        be_uint64_t host_ring_addr;
        be_uint16_t max_len;
        be_uint16_t flags;
        be_uint32_t nic_ring_addr;
    } __PACKED__;
    KASSERT(sizeof(ring_control_block) == 16);

    struct send_bd
    {
        be_uint64_t host_addr;
        be_uint16_t len;
        be_uint16_t flags;
        be_uint16_t rsrv;
        be_uint16_t vlan_tag;
    } __PACKED__;
    KASSERT(sizeof(send_bd) == 16);

    struct receive_bd
    {
        be_uint64_t host_addr;
        be_uint16_t index;
        be_uint16_t len;
        be_uint16_t type;
        be_uint16_t flags;
        be_uint16_t ip_csum;
        be_uint16_t tcp_udp_csum;
        be_uint16_t error_flags;
        be_uint16_t vlan_tag;
        be_uint32_t rss_hash;
        be_uint32_t opaque;
    } __PACKED__;
    KASSERT(sizeof(receive_bd) == 32);
}

#endif /* __KERNEL_NET_BCM57762_RING_H */
