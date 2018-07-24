#include "../net.h"
#include "net/ip/ip.h"
#include "tmock/tmock.h"

using kernel::_kassert;

// The original checksumming function we had in ip.h.
static uint16_t __UNUSED__ csum(const void* _p, size_t len)
{
    kassert(len % 2 == 0);
    size_t nwords     = len/2;
    uint32_t s        = 0;
    const uint16_t* p = (const uint16_t*)_p;
    for (size_t i=0; i<nwords; ++i)
        s += swap_uint(*p++);
    while (s >> 16)
        s = (s >> 16) + (s & 0xFFFF);
    return ~s;
}

class tmock_test
{
    TMOCK_TEST(test_no_data)
    {
        tmock::assert_equiv(net::ones_comp_csum(NULL,0,0),0xFFFFU);
    }

    TMOCK_TEST(test_zeroes_data)
    {
        uint8_t data[20];
        memset(data,0,sizeof(data));
        tmock::assert_equiv(net::ones_comp_csum(data,sizeof(data),0),0xFFFFU);
    }

    TMOCK_TEST(test_zeroes_data_incremental)
    {
        uint8_t data[2] = {0x00,0x00};
        uint16_t csum   = net::ones_comp_csum(data,sizeof(data),0);
        size_t offset   = 2;
        for (size_t i=0; i<10; ++i)
        {
            csum = net::ones_comp_csum(data,sizeof(data),offset,csum);
            offset += 2;
        }
        tmock::assert_equiv(csum,0xFFFFU);
    }

    TMOCK_TEST(test_ones_data)
    {
        uint8_t data[20];
        memset(data,0xFF,sizeof(data));
        tmock::assert_equiv(net::ones_comp_csum(data,sizeof(data),0),0x0000U);
    }

    TMOCK_TEST(test_ones_data_incremental)
    {
        uint8_t data[2] = {0xFF,0xFF};
        uint16_t csum   = net::ones_comp_csum(data,sizeof(data),0);
        size_t offset   = 2;
        for (size_t i=0; i<10; ++i)
        {
            csum = net::ones_comp_csum(data,sizeof(data),offset,csum);
            offset += 2;
        }
        tmock::assert_equiv(csum,0x0000U);
    }

    TMOCK_TEST(test_actual_data)
    {
        uint8_t data1[] = {0x00, 0x14, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0A,
                           0x00, 0x00, 0x00};
        uint8_t data2[] = {0x00, 0x50, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x48, 0x69};
        KASSERT(sizeof(data1) + sizeof(data2) == 22);

        ipv4::addr src_ip{192,168,0,31};
        ipv4::addr dst_ip{192,168,0,30};
        be_uint16_t proto = 6;
        be_uint16_t len   = sizeof(data1) + sizeof(data2);

        uint16_t csum;
        size_t offset = 0;
        csum = net::ones_comp_csum(&src_ip,sizeof(src_ip),offset);
        offset += sizeof(src_ip);
        csum = net::ones_comp_csum(&dst_ip,sizeof(dst_ip),offset,csum);
        offset += sizeof(dst_ip);
        csum = net::ones_comp_csum(&proto,sizeof(proto),offset,csum);
        offset += sizeof(proto);
        csum = net::ones_comp_csum(&len,sizeof(len),offset,csum);
        offset += sizeof(len);
        csum = net::ones_comp_csum(data1,sizeof(data1),offset,csum);
        offset += sizeof(data1);
        csum = net::ones_comp_csum(data2,sizeof(data2),offset,csum);

        tmock::assert_equiv(csum,0xC5C1U);
    }
};

TMOCK_MAIN();
