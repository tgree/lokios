#ifndef __MM_BUDDY_H
#define __MM_BUDDY_H

#include "mm.h"
#include "kern/cxx_exception.h"
#include <string.h>

#define BUDDY_PAGE_SIZE_LOG2    12U
#define BUDDY_PAGE_SIZE         (1UL << BUDDY_PAGE_SIZE_LOG2)

namespace kernel
{
    struct buddy_oom_exception : public kernel::message_exception
    {
        constexpr buddy_oom_exception():
            kernel::message_exception("buddy oom") {}
    };

    // Buddy helpers.
    constexpr size_t buddy_order_for_len(size_t len)
    {
        if (len <= BUDDY_PAGE_SIZE)
            return 0;
        return ulog2(ceil_pow2(len)) - BUDDY_PAGE_SIZE_LOG2;
    }
    constexpr size_t buddy_len_for_order(size_t order)
    {
        return (BUDDY_PAGE_SIZE << order);
    }
    KASSERT(buddy_order_for_len(0*BUDDY_PAGE_SIZE+1)  == 0);
    KASSERT(buddy_order_for_len(1*BUDDY_PAGE_SIZE-1)  == 0);
    KASSERT(buddy_order_for_len(1*BUDDY_PAGE_SIZE)    == 0);
    KASSERT(buddy_order_for_len(1*BUDDY_PAGE_SIZE+1)  == 1);
    KASSERT(buddy_order_for_len(2*BUDDY_PAGE_SIZE-1)  == 1);
    KASSERT(buddy_order_for_len(2*BUDDY_PAGE_SIZE)    == 1);
    KASSERT(buddy_order_for_len(2*BUDDY_PAGE_SIZE+1)  == 2);
    KASSERT(buddy_order_for_len(4*BUDDY_PAGE_SIZE-1)  == 2);
    KASSERT(buddy_order_for_len(4*BUDDY_PAGE_SIZE)    == 2);
    KASSERT(buddy_order_for_len(4*BUDDY_PAGE_SIZE+1)  == 3);
    KASSERT(buddy_order_for_len(8*BUDDY_PAGE_SIZE-1)  == 3);
    KASSERT(buddy_order_for_len(8*BUDDY_PAGE_SIZE)    == 3);
    KASSERT(buddy_order_for_len(8*BUDDY_PAGE_SIZE+1)  == 4);
    KASSERT(buddy_len_for_order(0) == BUDDY_PAGE_SIZE*1);
    KASSERT(buddy_len_for_order(1) == BUDDY_PAGE_SIZE*2);
    KASSERT(buddy_len_for_order(2) == BUDDY_PAGE_SIZE*4);

    // Buddy physical allocators - everything boils down to these.
    dma_addr64 buddy_palloc_by_order(size_t order);
    void buddy_pfree_by_order(dma_addr64 d, size_t order);
    void buddy_ppopulate(dma_addr64 d, size_t order);

    // Buddy virtual allocators.
    inline void* buddy_alloc_by_order(size_t order)
    {
        return kernel::phys_to_virt(buddy_palloc_by_order(order));
    }
    inline void* buddy_alloc_by_len(size_t len)
    {
        return buddy_alloc_by_order(buddy_order_for_len(len));
    }
    inline void* buddy_zalloc_by_order(size_t order)
    {
        void* p = buddy_alloc_by_order(order);
        memset(p,0,1ULL<<(order+BUDDY_PAGE_SIZE_LOG2));
        return p;
    }
    inline void buddy_free_by_order(void* p, size_t order)
    {
        buddy_pfree_by_order(virt_to_phys(p),order);
    }
    inline void buddy_free_by_len(void* p, size_t len)
    {
        buddy_free_by_order(p,buddy_order_for_len(len));
    }
    inline void buddy_populate(void* p, size_t order)
    {
        buddy_ppopulate(virt_to_phys(p),order);
    }

    size_t buddy_count_free();
    size_t buddy_count_total();

    size_t buddy_init(dma_addr64 dma_base, size_t len, dma_addr64 bitmap_base);

    struct buddy_block
    {
        const size_t len;
        void* const addr;
        buddy_block(size_t len):
            len(ceil_pow2(len)),
            addr(buddy_alloc_by_len(len))
        {
        }
        ~buddy_block()
        {
            buddy_free_by_len(addr,len);
        }
    };
}

#endif /* __MM_BUDDY_H */
