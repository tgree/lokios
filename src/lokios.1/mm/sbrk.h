#ifndef __KERNEL_SBRK_H
#define __KERNEL_SBRK_H

#include "mm.h"
#include "kernel/types.h"

namespace kernel
{
    dma_addr64 sbrk(size_t n);
    dma_addr64 get_sbrk();
    dma_addr64 get_sbrk_limit();
    void set_sbrk(dma_addr64);
    void set_sbrk_limit(dma_addr64 new_lim);
    void freeze_sbrk();
    void init_sbrk(dma_addr64 pos);

    template<size_t Len>
    struct sbrk_allocator
    {
        constexpr static size_t len   = Len;
        constexpr static bool movable = false;
        inline void* alloc()      {return phys_to_virt(sbrk(len));}
        inline void free(void* p) {}
    };
}

#endif /* __KERNEL_SBRK_H */
