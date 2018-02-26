#ifndef __KERNEL_ARGS_H
#define __KERNEL_ARGS_H

#include "mm/e820.h"
#include "types.h"

namespace kernel
{
    struct kernel_args
    {
        const e820_map* e820_base;
        dma_addr64      vga_base;
    };

    extern const kernel_args* kargs;
}

#endif /* __KERNEL_ARGS_H */
