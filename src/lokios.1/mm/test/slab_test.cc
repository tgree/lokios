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
        kernel::slab s(32);
    }

    TMOCK_TEST(test_single_alloc_works)
    {
        {
            kernel::slab s(8);
            kassert(page_alloc_count == 0);
            void* e = s.alloc();
            kassert(e != NULL);
            kassert(page_alloc_count == 1);
            kassert(page_free_count  == 0);
        }
        kassert(page_free_count == 1);
    }

    static void test_slab_many_allocs(uint16_t elem_size)
    {
        size_t alloc_count = 10000;
        size_t page_elem_count =
            kernel::slab_page::elem_count_for_elem_size(elem_size);
        auto expected_pages = kernel::ceil_div(alloc_count,page_elem_count);

        {
            kernel::slab s(elem_size);
            std::set<void*> elems;
            for (size_t i=0; i<alloc_count; ++i)
                elems.insert(s.alloc());
            kassert(elems.size() == alloc_count);
            kassert(page_alloc_count == expected_pages);
        }
        kassert(page_alloc_count == expected_pages);
        kassert(page_free_count  == expected_pages);
    }

    TMOCK_TEST(test_slab16_many_allocs)
    {
        test_slab_many_allocs(16);
    }

    static void test_slab_many_zallocs_and_frees(uint16_t elem_size)
    {
        size_t alloc_count = 10000;
        size_t page_elem_count =
            kernel::slab_page::elem_count_for_elem_size(elem_size);
        auto expected_pages = kernel::ceil_div(alloc_count,page_elem_count);

        {
            kernel::slab s(elem_size);
            std::set<void*> elems;
            for (size_t i=0; i<alloc_count; ++i)
                elems.insert(s.zalloc());
            kassert(elems.size()     == alloc_count);
            kassert(page_alloc_count == expected_pages);
            kassert(page_free_count  == 0);
            for (void* e : elems)
                s.free(e);
            elems.clear();
            for (size_t i=0; i<alloc_count; ++i)
                elems.insert(s.zalloc());
            kassert(elems.size()     == alloc_count);
            kassert(page_alloc_count == expected_pages);
            kassert(page_free_count  == 0);
        }
        kassert(page_alloc_count == expected_pages);
        kassert(page_free_count  == expected_pages);
    }

    TMOCK_TEST(test_slab16_many_zallocs_and_frees)
    {
        test_slab_many_zallocs_and_frees(16);
    }
};

TMOCK_MAIN();
