#include "../sort.h"
#include "../kmath.h"
#include "kernel/kassert.h"
#include "tmock/tmock.h"
#include <string.h>

void
kernel::panic(const char* s) noexcept
{
    tmock::abort(s);
}

class tmock_test
{
    TMOCK_TEST(test_some_integers)
    {
        const int sorted[]   = {1,2,3,4,5,6};
              int unsorted[] = {1,3,4,2,6,5};

        kernel::kassert(kernel::nelems(unsorted) == kernel::nelems(sorted));
        kernel::kassert(kernel::nelems(unsorted) == 6);
        kernel::sort::quicksort(unsorted);

        for (size_t i=0; i < kernel::nelems(unsorted); ++i)
            kernel::kassert(unsorted[i] == sorted[i]);
    }

    TMOCK_TEST(test_some_integers_flex_array)
    {
        const int sorted[]   = {1,2,3,4,5,6};
              int unsorted[kernel::nelems(sorted)];
        for (size_t i=0; i<kernel::nelems(sorted); ++i)
            unsorted[i] = sorted[kernel::nelems(sorted) - 1 - i];
        kernel::kassert(kernel::nelems(unsorted) == kernel::nelems(sorted));
        kernel::kassert(kernel::nelems(unsorted) == 6);
        kernel::sort::quicksort(unsorted);

        for (size_t i=0; i < kernel::nelems(unsorted); ++i)
            kernel::kassert(unsorted[i] == sorted[i]);
    }
};

TMOCK_MAIN();
