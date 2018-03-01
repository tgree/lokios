#ifndef __KERNEL_NET_ARP_H
#define __KERNEL_NET_ARP_H

#include "eth.h"
#include "ip.h"

namespace arp
{
    template<typename hw_traits, typename proto_traits>
    struct payload
    {
        typedef typename hw_traits::addr_type       hw_addr;
        typedef typename proto_traits::addr_type    proto_addr;

        be_uint16_t htype;
        be_uint16_t ptype;
        uint8_t     hlen;
        uint8_t     plen;
        be_uint16_t oper;
        hw_addr     sha;
        proto_addr  spa;
        hw_addr     tha;
        proto_addr  tpa;
    } __PACKED__;

    template<typename hw_traits, typename proto_traits>
    struct frame
    {
        typedef typename hw_traits::header_type header_type;
        
        header_type                     hdr;
        payload<hw_traits,proto_traits> msg;
    } __PACKED__;

    typedef frame<eth::net_traits,ipv4::net_traits> ipv4_arp_frame;
    KASSERT(sizeof(ipv4_arp_frame) == 42);
}

#endif /* __KERNEL_NET_ARP_H */
