#include "../buddy_allocator.h"
#include <stdlib.h>

size_t buddy_alloc_count;
size_t buddy_free_count;

dma_addr64
kernel::buddy_palloc(size_t order)
{
    void* p;
    kassert(posix_memalign(&p,(PAGE_SIZE << order),(PAGE_SIZE << order)) == 0);
    ++buddy_alloc_count;
    return virt_to_phys(p);
}

void
kernel::buddy_pfree(dma_addr64 d, size_t order)
{
    kassert(((uintptr_t)d & ((1<<(order+12))-1)) == 0);
    free(phys_to_virt(d));
    ++buddy_free_count;
}

size_t
kernel::buddy_count_free()
{
    return 0;
}

