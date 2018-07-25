#include "../header.h"
#include "tmock/tmock.h"

class tmock_test
{
    TMOCK_TEST(test_flags_bitfield)
    {
        tcp::header h;

        h.flags_offset = 0;
        h.ns           = 1;
        tmock::assert_equiv(h.flags_offset,0x0100U);

        h.flags_offset = 0;
        h.rsrv         = 7;
        tmock::assert_equiv(h.flags_offset,0x0E00U);

        h.flags_offset = 0;
        h.offset       = 0xF;
        tmock::assert_equiv(h.flags_offset,0xF000U);

        h.flags_offset = 0;
        h.fin          = 1;
        tmock::assert_equiv(h.flags_offset,0x0001U);

        h.flags_offset = 0;
        h.syn          = 1;
        tmock::assert_equiv(h.flags_offset,0x0002U);

        h.flags_offset = 0;
        h.rst          = 1;
        tmock::assert_equiv(h.flags_offset,0x0004U);

        h.flags_offset = 0;
        h.psh          = 1;
        tmock::assert_equiv(h.flags_offset,0x0008U);

        h.flags_offset = 0;
        h.ack          = 1;
        tmock::assert_equiv(h.flags_offset,0x0010U);

        h.flags_offset = 0;
        h.urg          = 1;
        tmock::assert_equiv(h.flags_offset,0x0020U);

        h.flags_offset = 0;
        h.ece          = 1;
        tmock::assert_equiv(h.flags_offset,0x0040U);

        h.flags_offset = 0;
        h.cwr          = 1;
        tmock::assert_equiv(h.flags_offset,0x0080U);
    }
};

TMOCK_MAIN();
