#include "../buddy_allocator.h"
#include <stdlib.h>
#include <unordered_map>

size_t buddy_alloc_count;
size_t buddy_free_count;

static std::unordered_map<dma_addr64,size_t>* order_map;

dma_addr64
kernel::buddy_palloc_by_order(size_t order)
{
    if (!order_map)
        order_map = new std::unordered_map<dma_addr64,size_t>();

    void* p;
    kassert(posix_memalign(&p,(PAGE_SIZE << order),(PAGE_SIZE << order)) == 0);
    ++buddy_alloc_count;
    dma_addr64 d = virt_to_phys(p);
    kassert(order_map->find(d) == order_map->end());
    order_map->emplace(std::make_pair(d,order));
    return d;
}

void
kernel::buddy_pfree_by_order(dma_addr64 d, size_t order)
{
    kassert(((uintptr_t)d & ((1<<(order+12))-1)) == 0);
    auto iter = order_map->find(d);
    kassert(iter != order_map->end());
    kassert(iter->second == order);
    order_map->erase(iter);
    free(phys_to_virt(d));
    ++buddy_free_count;
}

size_t
kernel::buddy_count_free()
{
    return 0;
}

