#include "../header.h"
#include "../tcp.h"
#include "tmock/tmock.h"

using namespace tcp;

class tmock_test
{
    TMOCK_TEST(test_flags_bitfield)
    {
        tcp::header h;

        h.flags_offset = 0;
        h.ns           = 1;
        tmock::assert_equiv(h.flags_offset,0x0100U);
        tmock::assert_equiv(h.flags_offset,tcp::FNS);

        h.flags_offset = 0;
        h.rsrv         = 7;
        tmock::assert_equiv(h.flags_offset,0x0E00U);

        h.flags_offset = 0;
        h.offset       = 0xF;
        tmock::assert_equiv(h.flags_offset,0xF000U);

        h.flags_offset = 0;
        h.fin          = 1;
        tmock::assert_equiv(h.flags_offset,0x0001U);
        tmock::assert_equiv(h.flags_offset,tcp::FFIN);

        h.flags_offset = 0;
        h.syn          = 1;
        tmock::assert_equiv(h.flags_offset,0x0002U);
        tmock::assert_equiv(h.flags_offset,tcp::FSYN);

        h.flags_offset = 0;
        h.rst          = 1;
        tmock::assert_equiv(h.flags_offset,0x0004U);
        tmock::assert_equiv(h.flags_offset,tcp::FRST);

        h.flags_offset = 0;
        h.psh          = 1;
        tmock::assert_equiv(h.flags_offset,0x0008U);
        tmock::assert_equiv(h.flags_offset,tcp::FPSH);

        h.flags_offset = 0;
        h.ack          = 1;
        tmock::assert_equiv(h.flags_offset,0x0010U);
        tmock::assert_equiv(h.flags_offset,tcp::FACK);

        h.flags_offset = 0;
        h.urg          = 1;
        tmock::assert_equiv(h.flags_offset,0x0020U);
        tmock::assert_equiv(h.flags_offset,tcp::FURG);

        h.flags_offset = 0;
        h.ece          = 1;
        tmock::assert_equiv(h.flags_offset,0x0040U);
        tmock::assert_equiv(h.flags_offset,tcp::FECE);

        h.flags_offset = 0;
        h.cwr          = 1;
        tmock::assert_equiv(h.flags_offset,0x0080U);
        tmock::assert_equiv(h.flags_offset,tcp::FCWR);
    }

    TMOCK_TEST(test_SEQ)
    {
        tcp::header h;
        h.init(SEQ{12345});
        tmock::assert_equiv((uint32_t)h.seq_num,12345U);
    }

    TMOCK_TEST(test_ACK)
    {
        tcp::header h;
        h.init(ACK{54321});
        tmock::assert_equiv((uint32_t)h.ack_num,54321U);
    }

    TMOCK_TEST(test_CTL)
    {
        tcp::header h;

        h.init(CTL{FFIN});
        tmock::assert_equiv((uint16_t)(h.flags_offset & 0x0FFF),FFIN);
        tmock::assert_equiv(h.fin,1U);

        h.format(CTL{FSYN});
        tmock::assert_equiv((uint16_t)(h.flags_offset & 0x0FFF),FSYN);
        tmock::assert_equiv(h.syn,1U);

        h.format(CTL{FRST});
        tmock::assert_equiv((uint16_t)(h.flags_offset & 0x0FFF),FRST);
        tmock::assert_equiv(h.rst,1U);

        h.format(CTL{FPSH});
        tmock::assert_equiv((uint16_t)(h.flags_offset & 0x0FFF),FPSH);
        tmock::assert_equiv(h.psh,1U);

        h.format(CTL{FACK});
        tmock::assert_equiv((uint16_t)(h.flags_offset & 0x0FFF),FACK);
        tmock::assert_equiv(h.ack,1U);

        h.format(CTL{FURG});
        tmock::assert_equiv((uint16_t)(h.flags_offset & 0x0FFF),FURG);
        tmock::assert_equiv(h.urg,1U);

        h.format(CTL{FECE});
        tmock::assert_equiv((uint16_t)(h.flags_offset & 0x0FFF),FECE);
        tmock::assert_equiv(h.ece,1U);

        h.format(CTL{FCWR});
        tmock::assert_equiv((uint16_t)(h.flags_offset & 0x0FFF),FCWR);
        tmock::assert_equiv(h.cwr,1U);

        h.format(CTL{FNS});
        tmock::assert_equiv((uint16_t)(h.flags_offset & 0x0FFF),FNS);
        tmock::assert_equiv(h.ns,1U);
    }

    TMOCK_TEST(test_SPORT)
    {
        tcp::header h;
        h.init(SPORT{1234});
        tmock::assert_equiv(h.src_port,1234U);
    }

    TMOCK_TEST(test_DPORT)
    {
        tcp::header h;
        h.init(DPORT{1234});
        tmock::assert_equiv(h.dst_port,1234U);
    }

    TMOCK_TEST(test_SIP)
    {
        tcp::ipv4_tcp_headers h;
        h.init(SIP{1,2,3,4});
        tmock::assert_equiv(h.ip.src_ip,ipv4::addr{1,2,3,4});
    }

    TMOCK_TEST(test_DIP)
    {
        tcp::ipv4_tcp_headers h;
        h.init(DIP{1,2,3,4});
        tmock::assert_equiv(h.ip.dst_ip,ipv4::addr{1,2,3,4});
    }

    TMOCK_TEST(test_WS)
    {
        tcp::header h;
        h.init(WS{0x00100000,0});
        tmock::assert_equiv(h.window_size,0xFFFFU);

        h.format(WS{0x00100000,8});
        tmock::assert_equiv(h.window_size,0x1000U);
    }

    TMOCK_TEST(test_all)
    {
        tcp::ipv4_tcp_headers h;
        h.init(SEQ{0x11111111},ACK{0x22222222},CTL{0x333},
               SIP{0x44,0x44,0x44,0x44},DIP{0x55,0x55,0x55,0x55},
               SPORT{0x6666},DPORT{0x7777});
        tmock::assert_equiv((uint32_t)h.tcp.seq_num,0x11111111U);
        tmock::assert_equiv((uint32_t)h.tcp.ack_num,0x22222222U);
        tmock::assert_equiv((uint16_t)h.tcp.flags_offset & 0x0FFF,0x333U);
        tmock::assert_equiv(h.ip.src_ip,ipv4::addr{0x44,0x44,0x44,0x44});
        tmock::assert_equiv(h.ip.dst_ip,ipv4::addr{0x55,0x55,0x55,0x55});
        tmock::assert_equiv((uint16_t)h.tcp.src_port,0x6666U);
        tmock::assert_equiv((uint16_t)h.tcp.dst_port,0x7777U);
    }
};

TMOCK_MAIN();
