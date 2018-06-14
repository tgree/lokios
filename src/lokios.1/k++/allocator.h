#ifndef __KERNEL_ALLOCATOR_H
#define __KERNEL_ALLOCATOR_H

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
}

#endif /* __KERNEL_ALLOCATOR_H */
