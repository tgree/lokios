#include "../page.h"
#include <stdlib.h>

size_t page_alloc_count;
size_t page_free_count;

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

size_t
kernel::page_count_free()
{
    return 0;
}
