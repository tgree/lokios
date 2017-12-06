#ifndef __KERNEL_ARGS_H
#define __KERNEL_ARGS_H

#include "mm/e820.h"

namespace kernel
{
    struct kernel_args
    {
        const e820_map* e820_base;
        uint64_t        vga_base;
        uint64_t        highest_pte_val;
    };

    extern const kernel_args* kargs;
}

#endif /* __KERNEL_ARGS_H */
