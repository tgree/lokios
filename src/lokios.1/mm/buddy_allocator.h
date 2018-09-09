#ifndef __MM_BUDDY_ALLOCATOR_H
#define __MM_BUDDY_ALLOCATOR_H

#include "page.h"
#include "mm.h"
#include "kern/cxx_exception.h"
#include <new>

#define BUDDY_ALLOCATOR_MAX_ORDER   32

namespace kernel
{
    template<size_t BPAGE_SIZE_LOG2>
    struct buddy_allocator_params
    {
        static constexpr const size_t bpage_size_log2 = BPAGE_SIZE_LOG2;
        static constexpr const size_t bpage_size      = (1ULL<<bpage_size_log2);
        const size_t        L;
        const size_t        k;
        const dma_addr64    B;
        const size_t        Bppfn;
        const dma_addr64    E;
        const size_t        M;

        constexpr size_t get_inuse_bitmask_size() const
        {
            return ceil_div(1ULL << (M+1),2ULL*8);
        }

        constexpr size_t index_for_ppfn(size_t ppfn, size_t order) const
        {
            size_t offset = ((ppfn - Bppfn) >> order);
            return (1ULL << (M+1)) - (1ULL << (M+1 - order)) + offset;
        }

        constexpr size_t index_for_paddr(dma_addr64 a, size_t order) const
        {
            return index_for_ppfn(a >> bpage_size_log2,order);
        }

        constexpr buddy_allocator_params(dma_addr64 dma_base, size_t len):
            L(ceil_pow2(len)),
            k(ulog2(L)),
            B(round_down_to_nearest_multiple(dma_base,L)),
            Bppfn(B >> bpage_size_log2),
            E(round_up_to_nearest_multiple(dma_base+len,L)),
            M(k - bpage_size_log2 + (E == (B + 2*L)))
        {
        }
    };

    template<size_t BPAGE_SIZE_LOG2, typename OOMException>
    struct buddy_allocator
    {
        static constexpr const size_t bpage_size_log2 = BPAGE_SIZE_LOG2;
        static constexpr const size_t bpage_size      = (1ULL<<bpage_size_log2);
        KASSERT(bpage_size > sizeof(kernel::kdlink));

        struct bpage
        {
            kernel::kdlink  link;
            char            data[bpage_size - sizeof(kernel::kdlink)];
        };
        KASSERT(sizeof(bpage) == bpage_size);

        const buddy_allocator_params<bpage_size_log2> params;
        uint8_t* const                  inuse_bitmask;
        bpage*                          virt_base;
        kernel::kdlist_leaks<bpage>     order_list[BUDDY_ALLOCATOR_MAX_ORDER];
        const size_t                    first_ppfn;
        const size_t                    last_ppfn;
        size_t                          nfree_pages;
        size_t                          total_pages;

        inline bool toggle_inuse_ppfn_bit(size_t ppfn, size_t order)
        {
            // Toggles the inuse bit for this ppfn buddy pair and returns the
            // new value.  The new value will be true if both pages are inuse
            // or both pages are free.
            size_t index               = params.index_for_ppfn(ppfn,order)/2;
            size_t byte_index          = index/8;
            size_t bit_index           = index % 8;
            inuse_bitmask[byte_index] ^= (1ULL << bit_index);
            return (inuse_bitmask[byte_index] & (1ULL << bit_index));
        }

        inline bool toggle_inuse_paddr_bit(dma_addr64 addr, size_t order)
        {
            return toggle_inuse_ppfn_bit(addr/bpage_size,order);
        }

        inline void populate_pages(bpage* bp, size_t order)
        {
            total_pages += (1ULL<<order);
            free_pages(bp,order);
        }

        inline void populate_pages(dma_addr64 addr, size_t order)
        {
            kassert(order <= params.M);
            kassert((addr & ((1ULL<<order)-1)) == 0);

            bpage* bp = new(phys_to_virt(addr)) bpage;
            populate_pages(bp,order);
        }

        inline void free_pages(bpage* bp, size_t order)
        {
            kassert(order <= params.M);

            size_t ppfn = virt_to_phys(bp)/bpage_size;
            kassert(first_ppfn <= ppfn);
            kassert(ppfn <= last_ppfn);

            order_list[order].push_front(&bp->link);
            nfree_pages += (1ULL<<order);
            if (!toggle_inuse_ppfn_bit(ppfn,order))
                return;

            bpage* bp2    = (bpage*)((uintptr_t)bp ^
                                     (1ULL << (order+bpage_size_log2)));
            bpage* first  = MIN(bp,bp2);
            bpage* second = MAX(bp,bp2);
            second->link.unlink();
            second->~bpage();

            first->link.unlink();
            nfree_pages -= (2ULL<<order);
            free_pages(first,order+1);
        }

        inline void free_pages(dma_addr64 addr, size_t order)
        {
            kassert(order <= params.M);
            kassert((addr & ((1ULL<<order)-1)) == 0);

            bpage* bp = new(phys_to_virt(addr)) bpage;
            free_pages(bp,order);
        }

        inline dma_addr64 alloc_pages(size_t order)
        {
            kassert(order <= params.M);

            dma_addr64 paddr;
            if (!order_list[order].empty())
            {
                // The free list is non-empty.  This means the other buddy for
                // the head of the free list is already allocated.  After this
                // operation, both buddy pages should be allocated and toggle
                // should return true.
                bpage* bp = klist_front(order_list[order],link);
                order_list[order].pop_front();
                bp->~bpage();
                paddr = virt_to_phys(bp);
                kassert(toggle_inuse_paddr_bit(paddr,order));
            }
            else if(order < params.M)
            {
                // The free list is empty.  Split a higher-order page.  After
                // this operation, we'll return one of the split pages and
                // queue up the other on the free list.  Since only one of the
                // buddy pages will be allocated at this point, toggle should
                // return false.
                paddr             = alloc_pages(order+1);
                nfree_pages      += (2ULL<<order);
                dma_addr64 bpaddr = (paddr ^ (1ULL << (order+bpage_size_log2)));
                bpage* bbp        = new(phys_to_virt(bpaddr)) bpage;
                order_list[order].push_front(&bbp->link);
                kassert(!toggle_inuse_paddr_bit(paddr,order));
            }
            else
                throw OOMException();

            nfree_pages -= (1ULL<<order);
            return paddr;
        }
        
        buddy_allocator(dma_addr64 dma_base, size_t len, void* inuse_bitmask):
            params(dma_base,len),
            inuse_bitmask((uint8_t*)inuse_bitmask),
            virt_base((bpage*)phys_to_virt_maybe_0(params.B)),
            first_ppfn(dma_base/bpage_size),
            last_ppfn((dma_base + len - 1)/bpage_size),
            nfree_pages(0),
            total_pages(0)
        {
            kassert(params.M <= BUDDY_ALLOCATOR_MAX_ORDER);
            memset(inuse_bitmask,0xFF,params.get_inuse_bitmask_size());
        }
    };
}

#endif /* __MM_BUDDY_ALLOCATOR_H */
