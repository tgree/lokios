#ifndef __KERNEL_TYPES_H
#define __KERNEL_TYPES_H

#include "hdr/endian.h"
#include "hdr/types.h"
#include <stddef.h>
#include <stdint.h>

namespace kernel
{
    template<size_t Width, typename T>
    struct fat_register
    {
        volatile T  data;
        const char  padding[Width - sizeof(T)];

        void operator=(T v)    {data = v;}
        operator volatile T&() {return data;}
    };
}

typedef uint64_t dma_addr64;

#endif /* __KERNEL_TYPES_H */
