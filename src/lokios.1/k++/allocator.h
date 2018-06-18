#ifndef __KERNEL_ALLOCATOR_H
#define __KERNEL_ALLOCATOR_H

#include "kmath.h"

namespace kernel
{
    template<size_t Len>
    struct static_allocator
    {
        constexpr static size_t len   = Len;
        constexpr static bool movable = false;
        uint64_t                backing[ceil_div(Len,sizeof(uint64_t))];
        inline void* alloc()      {return backing;}
        inline void free(void* p) {kassert(p == backing);}
    };

    template<typename Allocator>
    struct raii_block
    {
        Allocator&  allocator;
        void*       addr;

        inline raii_block(Allocator& allocator):
            allocator(allocator),
            addr(allocator.alloc())
        {
        }

        inline raii_block(raii_block&& other):
            allocator(other.allocator),
            addr(other.addr)
        {
            kassert(allocator.movable);
            other.addr = allocator.alloc();
        }

        inline ~raii_block()
        {
            allocator.free(addr);
        }
    };
}

#endif /* __KERNEL_ALLOCATOR_H */
