#include "../page_table.h"
#include <tmock/tmock.h>
#include <hdr/compiler.h>
#include <string.h>

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
    kernel::kassert(posix_memalign(&p,PAGE_SIZE,PAGE_SIZE) == 0);
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
    TMOCK_TEST(test_page_table_iterator_all_empty_page_works)
    {
        uint64_t* p = (uint64_t*)kernel::page_alloc();
        memset(p,0,PAGE_SIZE);

        size_t count = 0;
        for (auto pte __UNUSED__ : kernel::page_table_iterator(p,0,0))
            ++count;
        TASSERT(count == 512);

        kernel::page_free(p);
    }

    TMOCK_TEST(test_page_table_present_iterator_empty_page_works)
    {
        uint64_t* p = (uint64_t*)kernel::page_alloc();
        memset(p,0,PAGE_SIZE);

        size_t count = 0;
        for (auto pte __UNUSED__ : kernel::page_table_present_iterator(p))
            ++count;
        TASSERT(count == 0);

        kernel::page_free(p);
    }

    TMOCK_TEST(test_page_table_leaf_iterator_empty_page_works)
    {
        uint64_t* p = (uint64_t*)kernel::page_alloc();
        memset(p,0,PAGE_SIZE);

        size_t count = 0;
        for (auto pte __UNUSED__ : kernel::page_table_leaf_iterator(p))
            ++count;
        TASSERT(count == 0);

        kernel::page_free(p);
    }

    TMOCK_TEST(test_page_table_nonleaf_iterator_empty_page_works)
    {
        uint64_t* p = (uint64_t*)kernel::page_alloc();
        memset(p,0,PAGE_SIZE);

        size_t count = 0;
        for (auto pte __UNUSED__ : kernel::page_table_nonleaf_iterator(p))
            ++count;
        TASSERT(count == 0);

        kernel::page_free(p);
    }

    TMOCK_TEST(test_empty_page_table_doesnt_leak)
    {
        TASSERT(page_alloc_count == 0);
        TASSERT(page_free_count == 0);

        {
            kernel::page_table pt;
            TASSERT(page_alloc_count == 1);
            TASSERT(page_free_count == 0);
        }

        TASSERT(page_alloc_count == 1);
        TASSERT(page_free_count == 1);
    }
};

TMOCK_MAIN();
