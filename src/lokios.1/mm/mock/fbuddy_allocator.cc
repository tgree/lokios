#include "../buddy_allocator.h"
#include <stdlib.h>

void*
kernel::buddy_alloc(size_t order)
{
    void* p;
    kassert(posix_memalign(&p,(PAGE_SIZE << order),(PAGE_SIZE << order)) == 0);
    return p;
}

void
kernel::buddy_free(void* p, size_t order)
{
    kassert(((uintptr_t)p & ((1<<(order+12))-1)) == 0);
    free(p);
}

size_t
kernel::buddy_count_free()
{
    return 0;
}

