#ifndef __KERNEL_ARGS_H
#define __KERNEL_ARGS_H

#include "types.h"

namespace kernel
{
    struct kernel_args
    {
        dma_addr64      e820_base;
        dma_addr64      vga_base;
        dma_addr64      kernel_end;
    };
}

#endif /* __KERNEL_ARGS_H */
