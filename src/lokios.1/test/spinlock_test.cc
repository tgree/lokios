#include "../spinlock.h"
#include "tmock/tmock.h"

using kernel::spinlock;
using kernel::_kassert;

void
kernel::panic(const char* s) noexcept
{
    tmock::abort(s);
}

class tmock_test
{
    TMOCK_TEST(test_create_destroy_works)
    {
        spinlock s;
        kassert(s.val == 0);
        kassert(s.owner == 0);
    }

    TMOCK_TEST_EXPECT_FAILURE(test_destroy_held_lock_aborts)
    {
        spinlock s;
        s.acquire();
    }

    TMOCK_TEST(test_with_works)
    {
        spinlock s;
        kassert(s.val == 0);
        with (s)
        {
            kassert(s.val == 1);
        }
        kassert(s.val == 0);
        kassert(s.owner == 0);
    }

    TMOCK_TEST_EXPECT_FAILURE(test_recursive_acquire_aborts)
    {
        spinlock s;
        s.acquire();
        s.acquire();
    }

    TMOCK_TEST(test_multiple_acquire_release_works)
    {
        spinlock s;
        with (s) {}
        with (s) {}
        with (s) {}
    }

    TMOCK_TEST(test_nested_spinlocks_work)
    {
        spinlock s1;
        spinlock s2;

        kassert(s1.val == 0);
        kassert(s2.val == 0);
        with (s1)
        {
            kassert(s1.val == 1);
            kassert(s2.val == 0);

            with (s2)
            {
                kassert(s1.val == 1);
                kassert(s2.val == 1);
            }

            kassert(s1.val == 1);
            kassert(s2.val == 0);
        }
        kassert(s1.val == 0);
        kassert(s2.val == 0);
    }

    TMOCK_TEST_EXPECT_FAILURE(test_wrong_owner_release_aborts)
    {
        spinlock s1;
        with (s1)
        {
            s1.owner = ~s1.owner;
        }
    }
};

TMOCK_MAIN();
