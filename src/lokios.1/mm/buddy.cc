#include "buddy.h"
#include "buddy_allocator.h"
#include "kern/spinlock.h"
#include "k++/deferred_init.h"

static kernel::spinlock buddy_pages_lock;
static kernel::deferred_global<
    kernel::buddy_allocator<kernel::buddy_oom_exception>> buddy_pages;

dma_addr64
kernel::buddy_palloc_by_order(size_t order)
{
    with (buddy_pages_lock)
    {
        return buddy_pages->alloc_pages(order);
    }
}

void
kernel::buddy_pfree_by_order(dma_addr64 d, size_t order)
{
    kassert(((uintptr_t)d & ((1<<(order+12))-1)) == 0);
    with (buddy_pages_lock)
    {
        buddy_pages->free_pages(d,order);
    }
}

void
kernel::buddy_ppopulate(dma_addr64 d, size_t order)
{
    kassert(((uintptr_t)d & ((1<<(order+12))-1)) == 0);
    with (buddy_pages_lock)
    {
        buddy_pages->populate_pages(d,order);
    }
}

size_t
kernel::buddy_count_free()
{
    return buddy_pages->nfree_pages;
}

size_t
kernel::buddy_count_total()
{
    return buddy_pages->total_pages;
}

size_t
kernel::buddy_init(dma_addr64 dma_base, size_t len, dma_addr64 bitmap_base)
{
    // Reserve memory for the buddy allocator bitmap.
    auto params  = buddy_allocator_params(dma_base,len);
    auto bmlen   = params.get_inuse_bitmask_size();
    void* bitmap = phys_to_virt(bitmap_base);
    buddy_pages.init(dma_base,len,bitmap);
    return bmlen;
}
