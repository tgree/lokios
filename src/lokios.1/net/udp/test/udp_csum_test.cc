#include "../udp.h"
#include "net/ip/ip.h"
#include "tmock/tmock.h"

using kernel::_kassert;

struct crappy_header
{
    uint8_t     data[12];
};

template<size_t N>
struct udp_ip_packet
{
    crappy_header   llhdr;
    ipv4::header    iphdr;
    udp::header     uhdr;
    uint8_t         data[N];
} __PACKED__;

class tmock_test
{
    TMOCK_TEST(test_actual_data)
    {
        udp_ip_packet<2> p;
        p.iphdr.src_ip  = ipv4::addr{152,1,51,27};
        p.iphdr.dst_ip  = ipv4::addr{152,14,94,75};
        p.iphdr.proto   = 0x11;
        p.uhdr.src_port = 0xA08F;
        p.uhdr.dst_port = 0x2694;
        p.uhdr.len      = sizeof(p.uhdr) + sizeof(p.data);
        p.uhdr.checksum = 0;
        p.data[0]       = 0x62;
        p.data[1]       = 0x62;
        kassert(p.uhdr.len == 10);

        net::tx_op op;
        op.nalps         = 1;
        op.alps[0].paddr = kernel::virt_to_phys(&p);
        op.alps[0].len   = sizeof(p);
        uint16_t csum    = udp::compute_checksum(&op,sizeof(p.llhdr));

        tmock::assert_equiv(csum,0x14DEU);
    }
};

TMOCK_MAIN();
