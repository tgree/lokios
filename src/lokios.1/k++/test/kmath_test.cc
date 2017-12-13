#include "k++/kmath.h"
#include "tmock/tmock.h"

using kernel::_kassert;
using kernel::round_up_pow2;
using kernel::round_down_pow2;

void
kernel::panic(const char* s) noexcept
{
    tmock::abort(s);
}

class tmock_Test
{
    TMOCK_TEST(test_round_up_pow2)
    {
        kassert(round_up_pow2(0,1024) == 0);
        kassert(round_up_pow2(1,1024) == 1024);
        kassert(round_up_pow2(1023,1024) == 1024);
        kassert(round_up_pow2(1024,1024) == 1024);
        kassert(round_up_pow2(1025,1024) == 2048);
        kassert(round_up_pow2(12345,1024) == 13312);

        kassert(round_up_pow2(10,32) == 32);
        kassert(round_up_pow2(10,8) == 16);

        kassert(round_up_pow2((uint64_t*)0x12345,0x0100) == (uint64_t*)0x12400);
    }
    
    TMOCK_TEST(test_round_down_pow2)
    {
        kassert(round_down_pow2(0,1024) == 0);
        kassert(round_down_pow2(1,1024) == 0);
        kassert(round_down_pow2(1023,1024) == 0);
        kassert(round_down_pow2(1024,1024) == 1024);
        kassert(round_down_pow2(1025,1024) == 1024);
        kassert(round_down_pow2(12345,1024) == 12288);

        kassert(round_down_pow2(100,32) == 96);
        kassert(round_down_pow2(100,8) == 96);

        kassert(round_down_pow2((uint64_t*)0x12345,0x0100)
                    == (uint64_t*)0x12300);
    }
};

TMOCK_MAIN();
