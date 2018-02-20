#ifndef __KERNEL_SBRK_H
#define __KERNEL_SBRK_H

#include "kernel/types.h"

namespace kernel
{
    dma_addr64 sbrk(size_t n);
    dma_addr64 get_sbrk();
    dma_addr64 get_sbrk_limit();
    void set_sbrk(dma_addr64);
    void set_sbrk_limit(dma_addr64 new_lim);
    void freeze_sbrk();
}

#endif /* __KERNEL_SBRK_H */
