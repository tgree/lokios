#include "../page.h"
#include "../page_flags.h"
#include <stdlib.h>

size_t page_alloc_count;
size_t page_free_count;

void*
kernel::page_alloc()
{
    // Allocate a 4K page that will work as a fake physical address (meaning
    // that it has to have zero high-order bits according to what is supported
    // by the x86 page table).
    void* p;
    kassert(posix_memalign(&p,PAGE_SIZE,PAGE_SIZE) == 0);
    kassert(((uintptr_t)p & ~PAGE_PADDR_MASK) == 0);
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
