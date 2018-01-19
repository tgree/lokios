#include "../slab.h"
#include "k++/kmath.h"
#include <tmock/tmock.h>
#include <set>

using kernel::_kassert;

static size_t page_alloc_count;
static size_t page_free_count;

void
kernel::panic(const char* s) noexcept
{
    tmock::abort(s);
}

void*
kernel::page_alloc()
{
    void* p;
    kassert(posix_memalign(&p,PAGE_SIZE,PAGE_SIZE) == 0);
    ++page_alloc_count;
    return p;
}

void
kernel::page_free(void* p)
{
    ++page_free_count;
    free(p);
}

class tmock_test
{
    TMOCK_TEST(test_empty_slab_works)
    {
        kernel::slab<32> s;
    }

    TMOCK_TEST(test_single_alloc_works)
    {
        {
            kernel::slab<8> s;
            kassert(page_alloc_count == 0);
            void* e = s.alloc();
            kassert(e != NULL);
            kassert(page_alloc_count == 1);
            kassert(page_free_count  == 0);
        }
        kassert(page_free_count == 1);
    }

    template<typename SlabType>
    static void test_slab_many_allocs()
    {
        constexpr size_t elem_count = 10000;
        auto expected_pages =
            kernel::ceil_div(elem_count,SlabType::page::elem_count);

        {
            SlabType s;
            std::set<void*> elems;
            for (size_t i=0; i<elem_count; ++i)
                elems.insert(s.alloc());
            kassert(elems.size() == elem_count);
            kassert(page_alloc_count == expected_pages);
        }
        kassert(page_alloc_count == expected_pages);
        kassert(page_free_count  == expected_pages);
    }

    TMOCK_TEST(test_slab16_many_allocs)
    {
        test_slab_many_allocs<kernel::slab<16>>();
    }

    template<typename SlabType>
    static void test_slab_many_allocs_and_frees()
    {
        constexpr size_t elem_count = 10000;
        auto expected_pages =
            kernel::ceil_div(elem_count,SlabType::page::elem_count);

        {
            SlabType s;
            std::set<void*> elems;
            for (size_t i=0; i<elem_count; ++i)
                elems.insert(s.alloc());
            kassert(elems.size()     == elem_count);
            kassert(page_alloc_count == expected_pages);
            kassert(page_free_count  == 0);
            for (void* e : elems)
                s.free(e);
            elems.clear();
            for (size_t i=0; i<elem_count; ++i)
                elems.insert(s.alloc());
            kassert(elems.size()     == elem_count);
            kassert(page_alloc_count == expected_pages);
            kassert(page_free_count  == 0);
        }
        kassert(page_alloc_count == expected_pages);
        kassert(page_free_count  == expected_pages);
    }

    TMOCK_TEST(test_slab16_many_allocs_and_frees)
    {
        test_slab_many_allocs_and_frees<kernel::slab<16>>();
    }
};

TMOCK_MAIN();
