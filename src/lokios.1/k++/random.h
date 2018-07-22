#ifndef __KERNEL_RANDOM_H
#define __KERNEL_RANDOM_H

#include <stdint.h>

namespace kernel
{
    uint32_t    random(uint32_t first = 0, uint32_t last = 0xFFFFFFFFU);
    void        random_seed(uint64_t val);
}

#endif /* __KERNEL_RANDOM_H */
