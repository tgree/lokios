#include "buddy_allocator.h"
#include "sbrk.h"
#include "../spinlock.h"
#include "k++/deferred_global.h"

static kernel::spinlock                                 buddy_pages_lock;
static kernel::deferred_global<kernel::buddy_allocator> buddy_pages;

void*
kernel::buddy_alloc(size_t order)
{
    with (buddy_pages_lock)
    {
        return phys_to_virt(buddy_pages->alloc_pages(order));
    }
}

void
kernel::buddy_free(void* p, size_t order)
{
    kassert(((uintptr_t)p & ((1<<(order+12))-1)) == 0);
    with (buddy_pages_lock)
    {
        buddy_pages->free_pages(virt_to_phys(p),order);
    }
}

size_t
kernel::buddy_count_free()
{
    return buddy_pages->nfree_pages;
}

void
kernel::buddy_init(dma_addr64 dma_base, size_t len)
{
    // Reserve memory for the buddy allocator bitmap.
    auto params = buddy_allocator_params(dma_base,len);
    void* bitmap = phys_to_virt(sbrk(params.get_inuse_bitmask_size()));
    buddy_pages.init(dma_base,len,bitmap);
}