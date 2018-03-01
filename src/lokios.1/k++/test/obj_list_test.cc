#include "../obj_list.h"
#include "tmock/tmock.h"

class tmock_test
{
    TMOCK_TEST(test_empty_obj_list_works)
    {
        kernel::obj_list<int> ol;
    }

    TMOCK_TEST(test_obj_list_push_back_works)
    {
        kernel::obj_list<int> ol;

        ol.push_back(3);
        ol.push_back(1);
        ol.push_back(4);

        tmock::assert_equiv(ol.size(),3U);

        ol.pop_all();
    }

    TMOCK_TEST(test_obj_list_front_works)
    {
        kernel::obj_list<int> ol;

        ol.push_back(3);
        ol.push_back(1);
        ol.push_back(4);

        tmock::assert_equiv(ol.front(),3);

        ol.pop_all();
    }

    TMOCK_TEST(test_obj_list_rbfl_works)
    {
        kernel::obj_list<int> ol;

        ol.push_back(3);
        ol.push_back(1);
        ol.push_back(4);
        ol.push_back(2);
        ol.push_back(5);

        static int vals[] = {3,1,4,2,5};
        int* vp = vals;
        for (int& o : ol)
            tmock::assert_equiv(o,*vp++);
        kernel::kassert(vp == &vals[5]);

        ol.pop_all();
    }

    TMOCK_TEST(test_obj_list_insert_works)
    {
        kernel::obj_list<int> ol;

        ol.push_back(3);
        ol.push_back(1);
        ol.push_back(4);
        ol.push_back(2);
        ol.push_back(5);

        auto i = ol.begin();
        ++i;
        ++i;
        ol.insert(i,11);

        static int vals[] = {3,1,11,4,2,5};
        int* vp = vals;
        for (int& o : ol)
            tmock::assert_equiv(o,*vp++);
        kernel::kassert(vp == &vals[6]);

        ol.pop_all();
    }

    TMOCK_TEST(test_obj_list_erase_works)
    {
        kernel::obj_list<int> ol;

        ol.push_back(3);
        ol.push_back(1);
        ol.push_back(4);
        ol.push_back(2);
        ol.push_back(5);

        auto i = ol.begin();
        ++i;
        ++i;
        ol.erase(i);

        static int vals[] = {3,1,2,5};
        int* vp = vals;
        for (int& o : ol)
            tmock::assert_equiv(o,*vp++);
        kernel::kassert(vp == &vals[4]);

        ol.pop_all();
    }

    TMOCK_TEST(test_obj_listlist_empty_rbfl_works)
    {
        kernel::obj_list<int> ol;
        size_t n = 0;
        for (int& o __attribute__((unused)) : ol)
            ++n;
        kernel::kassert(n == 0);
    }

    TMOCK_TEST(test_obj_list_one_elem_rbfl_works)
    {
        kernel::obj_list<int> ol;
        ol.push_back(12345);
        size_t n = 0;
        for (int& o : ol)
        {
            kernel::kassert(o == 12345);
            ++n;
        }
        kernel::kassert(n == 1);
        ol.pop_front();
    }
};

TMOCK_MAIN();
