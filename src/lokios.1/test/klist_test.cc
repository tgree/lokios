#include "../klist.h"
#include "tmock/tmock.h"

struct klinked_object
{
    kernel::klink   link;
    int             val;

    constexpr klinked_object(int val):val(val) {}
};

void
kernel::panic(const char* s) noexcept
{
    tmock::abort(s);
}

class tmock_test
{
    TMOCK_TEST(test_unused_klink_works)
    {
        kernel::klink kl;
    }

    TMOCK_TEST_EXPECT_FAILURE(test_klink_inuse_assert)
    {
        kernel::klink kl;
        kl.next = NULL;
    }

    TMOCK_TEST(test_empty_klist_works)
    {
        kernel::klist<klinked_object> kl;
    }

    TMOCK_TEST_EXPECT_FAILURE(test_klist_head_inuse_assert)
    {
        kernel::klist<klinked_object> kl;
        kl.head = (kernel::klink*)12345;
    }

    TMOCK_TEST_EXPECT_FAILURE(test_klist_tail_inuse_assert)
    {
        kernel::klist<klinked_object> kl;
        kl.tail = (kernel::klink*)12345;
    }

    TMOCK_TEST(test_klist_push_back_works)
    {
        klinked_object o1(1);
        klinked_object o2(2);
        klinked_object o3(3);
        klinked_object o4(4);
        klinked_object o5(5);
        kernel::klist<klinked_object> kl;

        kl.push_back(&o3.link);
        kl.push_back(&o1.link);
        kl.push_back(&o4.link);

        tmock::assert_equiv(kl.size(),3);

        kl.pop_all();
    }

    TMOCK_TEST(test_klist_rbfl_works)
    {
        klinked_object o1(1);
        klinked_object o2(2);
        klinked_object o3(3);
        klinked_object o4(4);
        klinked_object o5(5);
        kernel::klist<klinked_object> kl;

        kl.push_back(&o3.link);
        kl.push_back(&o1.link);
        kl.push_back(&o4.link);
        kl.push_back(&o2.link);
        kl.push_back(&o5.link);

        static int vals[] = {3,1,4,2,5};
        int* vp = vals;
        for (klinked_object& o : klist_elems(kl,link))
            tmock::assert_equiv(o.val,*vp++);
        kernel::kassert(vp == &vals[5]);

        kl.pop_all();
    }

    TMOCK_TEST(test_klist_empty_rbfl_works)
    {
        kernel::klist<klinked_object> kl;
        size_t n = 0;
        for (klinked_object& o __attribute__((unused)) : klist_elems(kl,link))
            ++n;
        kernel::kassert(n == 0);
    }

    TMOCK_TEST(test_klist_one_elem_rbfl_works)
    {
        kernel::klist<klinked_object> kl;
        klinked_object o1(12345);
        kl.push_back(&o1.link);
        size_t n = 0;
        for (klinked_object& o __attribute__((unused)) : klist_elems(kl,link))
        {
            kernel::kassert(o.val == 12345);
            ++n;
        }
        kernel::kassert(n == 1);
        kl.pop_front();
    }
};

TMOCK_MAIN();
