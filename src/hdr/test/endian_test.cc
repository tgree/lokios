#include "../endian.h"
#include <tmock/tmock.h>

class tmock_test
{
    TMOCK_TEST(test_be_uint64_t)
    {
        union
        {
            be_uint64_t be;
            uint8_t     c[sizeof(be)];
        } u = {be_uint64_t(0x0123456789ABCDEFUL)};
        tmock::assert_equiv(u.be,0x0123456789ABCDEFUL);
        tmock::assert_equiv(u.c[0],0x01);
        tmock::assert_equiv(u.c[1],0x23);
        tmock::assert_equiv(u.c[2],0x45);
        tmock::assert_equiv(u.c[3],0x67);
        tmock::assert_equiv(u.c[4],0x89);
        tmock::assert_equiv(u.c[5],0xAB);
        tmock::assert_equiv(u.c[6],0xCD);
        tmock::assert_equiv(u.c[7],0xEF);
    }

    TMOCK_TEST(test_be_uint32_t)
    {
        union
        {
            be_uint32_t be;
            uint8_t     c[sizeof(be)];
        } u = {be_uint32_t(0x89ABCDEFUL)};
        tmock::assert_equiv(u.be,0x89ABCDEFUL);
        tmock::assert_equiv(u.c[0],0x89);
        tmock::assert_equiv(u.c[1],0xAB);
        tmock::assert_equiv(u.c[2],0xCD);
        tmock::assert_equiv(u.c[3],0xEF);
    }

    TMOCK_TEST(test_be_uint16_t)
    {
        union
        {
            be_uint16_t be;
            uint8_t     c[sizeof(be)];
        } u = {be_uint16_t(0xCDEFUL)};
        tmock::assert_equiv(u.be,0xCDEFU);
        tmock::assert_equiv(u.c[0],0xCD);
        tmock::assert_equiv(u.c[1],0xEF);
    }

    TMOCK_TEST_EXPECT_FAILURE_SHOULD_PASS(test_le_uint64_t)
    {
        union
        {
            le_uint64_t le;
            uint8_t     c[sizeof(le)];
        } u = {le_uint64_t(0x0123456789ABCDEFUL)};
        tmock::assert_equiv(u.le,0x0123456789ABCDEFUL);
        tmock::assert_equiv(u.c[0],0xEF);
        tmock::assert_equiv(u.c[1],0xCD);
        tmock::assert_equiv(u.c[2],0xAB);
        tmock::assert_equiv(u.c[3],0x89);
        tmock::assert_equiv(u.c[4],0x67);
        tmock::assert_equiv(u.c[5],0x45);
        tmock::assert_equiv(u.c[6],0x23);
        tmock::assert_equiv(u.c[7],0x01);
    }

    TMOCK_TEST_EXPECT_FAILURE_SHOULD_PASS(test_le_uint32_t)
    {
        union
        {
            le_uint32_t le;
            uint8_t     c[sizeof(le)];
        } u = {le_uint32_t(0x89ABCDEFUL)};
        tmock::assert_equiv(u.le,0x89ABCDEFUL);
        tmock::assert_equiv(u.c[0],0xEF);
        tmock::assert_equiv(u.c[1],0xCD);
        tmock::assert_equiv(u.c[2],0xAB);
        tmock::assert_equiv(u.c[3],0x89);
    }

    TMOCK_TEST_EXPECT_FAILURE_SHOULD_PASS(test_le_uint16_t)
    {
        union
        {
            le_uint16_t le;
            uint8_t     c[sizeof(le)];
        } u = {le_uint16_t(0xCDEFUL)};
        tmock::assert_equiv(u.le,0xCDEFU);
        tmock::assert_equiv(u.c[0],0xEF);
        tmock::assert_equiv(u.c[1],0xCD);
    }
};

TMOCK_MAIN();
