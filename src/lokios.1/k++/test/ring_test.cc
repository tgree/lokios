#include "../ring.h"
#include "tmock/tmock.h"

using kernel::ring;

static uint32_t elems[32];

class tmock_test
{
    TMOCK_TEST(test_initially_empty)
    {
        ring r(elems);
        tmock::assert_equiv(r.nelems,NELEMS(elems));
        TASSERT(r.empty());
        TASSERT(!r.full());
        TASSERT(r.size() == 0);
    }

    TMOCK_TEST_EXPECT_FAILURE(test_empty_subscript_asserts)
    {
        ring r(elems);
        r[0];
    }

    TMOCK_TEST(test_fill_works)
    {
        ring r(elems);
        for (size_t i=0; i<NELEMS(elems); ++i)
        {
            TASSERT(!r.full());
            TASSERT(r.size() == i);
            r.emplace_back(i);
        }
        TASSERT(r.full());
        TASSERT(r.size() == NELEMS(elems));
    }

    TMOCK_TEST_EXPECT_FAILURE(test_overflow_asserts)
    {
        ring r(elems);
        for (size_t i=0; i<NELEMS(elems); ++i)
            r.emplace_back(i);
        r.emplace_back(12345);
    }

    TMOCK_TEST(test_pop_works)
    {
        ring r(elems,NELEMS(elems));
        for (size_t i=0; i<NELEMS(elems); ++i)
            r.emplace_back(i);
        r.pop_front(NELEMS(elems)/2);
        for (size_t i=0; i<NELEMS(elems)/2; ++i)
            tmock::assert_equiv(r[i],i + NELEMS(elems)/2);
    }

    TMOCK_TEST(test_wrap_works)
    {
        ring r(elems,NELEMS(elems));
        for (size_t i=0; i<NELEMS(elems); ++i)
            r.emplace_back(i);
        r.pop_front(NELEMS(elems)/2);
        for (size_t i=NELEMS(elems); i<3*NELEMS(elems)/2; ++i)
            r.emplace_back(i);
        for (size_t i=NELEMS(elems)/2; i<3*NELEMS(elems)/2; ++i)
            tmock::assert_equiv(r[i-NELEMS(elems)/2],i);
    }
};

TMOCK_MAIN();
