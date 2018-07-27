#include "../hash.h"
#include "tmock/tmock.h"

class tmock_test
{
    TMOCK_TEST(test_integral_hashes)
    {
        tmock::assert_equiv(hash::compute(1234),1234U);
        tmock::assert_equiv(hash::compute('c'),(size_t)'c');
    }

    TMOCK_TEST(test_string_hash)
    {
        tmock::assert_equiv(hash::compute("one"),  0x007C091DCC18779AU);
        tmock::assert_equiv(hash::compute("One"),  0x0058069D4411F06FU);
        tmock::assert_equiv(hash::compute("two"),  0x0084E9BC2CE7B706U);
        tmock::assert_equiv(hash::compute("three"),0x59DE8DF818B4C359U);
    }

    TMOCK_TEST(test_ptr_hash)
    {
        const uint32_t* v = (const uint32_t*)0x1234;
        tmock::assert_equiv(hash::compute(v),0x1234/sizeof(uint32_t));

        uint32_t* v2 = (uint32_t*)0x5678;
        tmock::assert_equiv(hash::compute(v2),0x5678/sizeof(uint32_t));

        struct blah
        {
            uint64_t    a;
            char        b;
        } __PACKED__;
        TASSERT(sizeof(blah) == 9);
        blah* b = (blah*)0x12345678;
        tmock::assert_equiv(hash::compute(b),0x12345678U/sizeof(blah));
    }
};

TMOCK_MAIN();
